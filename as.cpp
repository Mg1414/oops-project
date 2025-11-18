#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace car_rental {

struct CarRecord {
    std::string id;
    std::string model;
    std::string condition;
    double pricePerDay{0.0};
    std::string status{"Available"};
};

class CarRecordValidator {
public:
    bool validate(const CarRecord &record) const {
        static const std::vector<std::string> allowed{
            "excellent", "good", "fair", "minordamages", "majordamages"};

        if (record.id.empty() || record.model.empty()) {
            return false;
        }

        const bool isConditionValid = std::find(allowed.begin(), allowed.end(), record.condition) != allowed.end();
        if (!isConditionValid) {
            return false;
        }

        if (!std::isfinite(record.pricePerDay) || record.pricePerDay <= 0) {
            return false;
        }

        return true;
    }
};

class CarRecordParser {
public:
    explicit CarRecordParser(std::shared_ptr<CarRecordValidator> validator)
        : validator_(std::move(validator)) {}

    std::optional<CarRecord> parse(const std::string &line) const {
        if (line.empty()) {
            return std::nullopt;
        }

        std::vector<std::string> tokens;
        std::string token;
        std::stringstream ss(line);
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 4) {
            throw std::runtime_error("Malformed car record: " + line);
        }

        CarRecord record;
        if (tokens.size() >= 4) {
            record.id = tokens[0];
            record.model = tokens.size() > 4 ? tokens[1] : tokens[0];
            record.condition = tokens.size() > 4 ? tokens[2] : tokens[1];
            record.pricePerDay = std::stod(tokens.size() > 4 ? tokens[3] : tokens[2]);
            record.status = tokens.size() > 4 ? tokens[4] : (tokens.size() > 3 ? tokens[3] : "Available");
        } else {
            throw std::runtime_error("Malformed car record: " + line);
        }

        if (!validator_->validate(record)) {
            throw std::runtime_error("Validation failed for line: " + line);
        }

        return record;
    }

    std::string serialize(const CarRecord &record) const {
        if (!validator_->validate(record)) {
            throw std::runtime_error("Attempted to serialize invalid record: " + record.id);
        }

        std::ostringstream oss;
        oss << record.id << ',' << record.model << ',' << record.condition << ','
            << record.pricePerDay << ',' << record.status;
        return oss.str();
    }

private:
    std::shared_ptr<CarRecordValidator> validator_;
};

class TransactionalFileWriter {
public:
    explicit TransactionalFileWriter(std::string targetPath)
        : targetPath_(std::move(targetPath)) {}

    void write(const std::vector<std::string> &lines) const {
        namespace fs = std::filesystem;
        fs::path target(targetPath_);
        if (!target.has_parent_path()) {
            target = fs::current_path() / target;
        }
        if (!target.parent_path().empty()) {
            fs::create_directories(target.parent_path());
        }

        const fs::path temp = target; // overwritten with .tmp extension
        fs::path tmpPath = temp;
        tmpPath += ".tmp";

        std::ofstream output(tmpPath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            throw std::runtime_error("Unable to open temporary file for transactional write");
        }

        std::vector<char> buffer(1 << 16);
        output.rdbuf()->pubsetbuf(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        for (const auto &line : lines) {
            output << line << '\n';
        }
        output.flush();
        output.close();

        if (fs::exists(target)) {
            fs::remove(target);
        }
        fs::rename(tmpPath, target);
    }

private:
    std::string targetPath_;
};

class CarFilePipeline {
public:
    CarFilePipeline(std::string path, std::shared_ptr<CarRecordValidator> validator)
        : path_(std::move(path)), parser_(std::move(validator)) {}

    std::vector<CarRecord> readAll() const {
        std::vector<CarRecord> records;
        stream([&records](CarRecord &&record) {
            records.emplace_back(std::move(record));
        });
        return records;
    }

    void stream(const std::function<void(CarRecord &&)> &consumer) const {
        namespace fs = std::filesystem;
        fs::path filePath(path_);
        if (!fs::exists(filePath)) {
            return;
        }

        std::ifstream input(path_);
        if (!input.is_open()) {
            throw std::runtime_error("Unable to open " + path_);
        }

        std::vector<char> buffer(1 << 16);
        input.rdbuf()->pubsetbuf(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        std::string line;
        while (std::getline(input, line)) {
            try {
                auto record = parser_.parse(line);
                if (record.has_value()) {
                    consumer(std::move(*record));
                }
            } catch (const std::exception &ex) {
                std::cerr << "[WARN] Skipping line: " << ex.what() << std::endl;
            }
        }
    }

    void writeAll(const std::vector<CarRecord> &records) const {
        std::vector<std::string> lines;
        lines.reserve(records.size());
        for (const auto &record : records) {
            lines.push_back(parser_.serialize(record));
        }

        TransactionalFileWriter writer(path_);
        writer.write(lines);
    }

    const std::string &path() const { return path_; }

private:
    std::string path_;
    CarRecordParser parser_;
};

class StorageBackend {
public:
    virtual ~StorageBackend() = default;
    virtual std::vector<CarRecord> loadCars() = 0;
    virtual void persistCars(const std::vector<CarRecord> &records) = 0;
    virtual std::string name() const = 0;
};

class FileStorageBackend : public StorageBackend {
public:
    FileStorageBackend(std::string path, std::shared_ptr<CarRecordValidator> validator)
        : pipeline_(std::move(path), std::move(validator)) {}

    std::vector<CarRecord> loadCars() override {
        return pipeline_.readAll();
    }

    void persistCars(const std::vector<CarRecord> &records) override {
        pipeline_.writeAll(records);
    }

    std::string name() const override {
        return "file:" + pipeline_.path();
    }

private:
    CarFilePipeline pipeline_;
};

class MemoryStorageBackend : public StorageBackend {
public:
    MemoryStorageBackend() = default;
    explicit MemoryStorageBackend(std::vector<CarRecord> seed) : records_(std::move(seed)) {}

    std::vector<CarRecord> loadCars() override {
        return records_;
    }

    void persistCars(const std::vector<CarRecord> &records) override {
        records_ = records;
    }

    std::string name() const override {
        return "in-memory";
    }

private:
    std::vector<CarRecord> records_;
};

enum class BackendType { File, Memory };

class StorageBackendFactory {
public:
    static std::shared_ptr<StorageBackend> create(BackendType type,
                                                  const std::string &path,
                                                  std::shared_ptr<CarRecordValidator> validator) {
        switch (type) {
        case BackendType::File:
            return std::make_shared<FileStorageBackend>(path, std::move(validator));
        case BackendType::Memory:
            return std::make_shared<MemoryStorageBackend>();
        }
        throw std::invalid_argument("Unknown backend type");
    }
};

class CarRepository {
public:
    explicit CarRepository(std::shared_ptr<StorageBackend> backend)
        : backend_(std::move(backend)) {
        reload();
    }

    void reload() {
        records_.clear();
        auto loaded = backend_->loadCars();
        for (auto &record : loaded) {
            records_[record.id] = record;
        }
        dirty_ = false;
    }

    std::vector<CarRecord> all() const {
        std::vector<CarRecord> snapshot;
        snapshot.reserve(records_.size());
        for (const auto &pair : records_) {
            snapshot.push_back(pair.second);
        }
        std::sort(snapshot.begin(), snapshot.end(), [](const CarRecord &lhs, const CarRecord &rhs) {
            return lhs.id < rhs.id;
        });
        return snapshot;
    }

    std::vector<CarRecord> available() const {
        std::vector<CarRecord> output;
        for (const auto &pair : records_) {
            if (pair.second.status == "Available") {
                output.push_back(pair.second);
            }
        }
        std::sort(output.begin(), output.end(), [](const CarRecord &lhs, const CarRecord &rhs) {
            return lhs.id < rhs.id;
        });
        return output;
    }

    std::optional<CarRecord> find(const std::string &id) const {
        auto it = records_.find(id);
        if (it == records_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool upsert(const CarRecord &record) {
        records_[record.id] = record;
        dirty_ = true;
        return true;
    }

    bool update(const std::string &id, const std::function<void(CarRecord &)> &mutator) {
        auto it = records_.find(id);
        if (it == records_.end()) {
            return false;
        }
        mutator(it->second);
        dirty_ = true;
        return true;
    }

    void bulkUpsert(const std::vector<CarRecord> &records) {
        for (const auto &record : records) {
            records_[record.id] = record;
        }
        dirty_ = dirty_ || !records.empty();
    }

    void flush() {
        if (!dirty_) {
            return;
        }
        backend_->persistCars(all());
        dirty_ = false;
    }

    size_t totalRecords() const {
        return records_.size();
    }

    bool pendingChanges() const {
        return dirty_;
    }

private:
    std::shared_ptr<StorageBackend> backend_;
    std::unordered_map<std::string, CarRecord> records_;
    bool dirty_{false};
};

class RentalService {
public:
    explicit RentalService(CarRepository &repository)
        : repository_(repository) {}

    void addCar(const CarRecord &record) {
        if (repository_.find(record.id).has_value()) {
            throw std::runtime_error("Car with id " + record.id + " already exists");
        }
        repository_.upsert(record);
        repository_.flush();
    }

    std::vector<CarRecord> listAvailable(size_t limit) const {
        auto cars = repository_.available();
        if (cars.size() > limit) {
            cars.resize(limit);
        }
        return cars;
    }

    bool rentCar(const std::string &carId, const std::string &userId, double &amountDue) {
        auto existing = repository_.find(carId);
        if (!existing.has_value()) {
            return false;
        }
        if (existing->status != "Available") {
            return false;
        }

        amountDue = existing->pricePerDay;
        return repository_.update(carId, [&](CarRecord &record) {
            record.status = "Rented by the user ID: " + userId;
        }) && flush();
    }

    bool returnCar(const std::string &carId) {
        bool updated = repository_.update(carId, [](CarRecord &record) {
            record.status = "Available";
        });
        if (!updated) {
            return false;
        }
        return flush();
    }

    void ingest(const std::vector<CarRecord> &records) {
        repository_.bulkUpsert(records);
        repository_.flush();
    }

    size_t totalRecords() const {
        return repository_.totalRecords();
    }

    void save() {
        repository_.flush();
    }

private:
    bool flush() {
        repository_.flush();
        return true;
    }

    CarRepository &repository_;
};

struct BatchMetrics {
    size_t processedRecords{0};
    size_t batches{0};
    std::chrono::milliseconds duration{0};
};

class BatchProcessor {
public:
    BatchProcessor(CarRepository &repository, std::shared_ptr<CarRecordValidator> validator)
        : repository_(repository), validator_(std::move(validator)) {}

    BatchMetrics ingest(const std::string &path, size_t chunkSize) {
        if (chunkSize == 0) {
            throw std::invalid_argument("chunkSize must be greater than zero");
        }

        BatchMetrics metrics;
        const auto start = std::chrono::steady_clock::now();
        CarFilePipeline pipeline(path, validator_);
        std::vector<CarRecord> buffer;
        buffer.reserve(chunkSize);

        pipeline.stream([&](CarRecord &&record) {
            buffer.emplace_back(std::move(record));
            ++metrics.processedRecords;
            if (buffer.size() >= chunkSize) {
                repository_.bulkUpsert(buffer);
                repository_.flush();
                buffer.clear();
                ++metrics.batches;
            }
        });

        if (!buffer.empty()) {
            repository_.bulkUpsert(buffer);
            repository_.flush();
            buffer.clear();
            ++metrics.batches;
        }

        const auto end = std::chrono::steady_clock::now();
        metrics.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        return metrics;
    }

private:
    CarRepository &repository_;
    std::shared_ptr<CarRecordValidator> validator_;
};

class SyntheticDatasetGenerator {
public:
    explicit SyntheticDatasetGenerator(std::shared_ptr<CarRecordValidator> validator)
        : validator_(std::move(validator)) {}

    std::vector<CarRecord> generate(size_t count) const {
        std::vector<CarRecord> records;
        records.reserve(count);
        std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> priceDist(1800.0, 7500.0);
        std::vector<std::string> conditions{ "excellent", "good", "fair", "minordamages", "majordamages" };
        std::vector<std::string> models{ "Atlas", "Falcon", "Nimbus", "Aurora", "Vertex" };

        for (size_t i = 0; i < count; ++i) {
            CarRecord record;
            record.id = "car-" + std::to_string(i + 1);
            record.model = models[i % models.size()];
            record.condition = conditions[i % conditions.size()];
            record.pricePerDay = std::round(priceDist(rng));
            records.emplace_back(std::move(record));
        }
        return records;
    }

    void toFile(const std::string &path, size_t count) const {
        auto records = generate(count);
        CarFilePipeline pipeline(path, validator_);
        pipeline.writeAll(records);
    }

private:
    std::shared_ptr<CarRecordValidator> validator_;
};

struct CommandContext {
    RentalService &service;
    SyntheticDatasetGenerator &generator;
    BatchProcessor &batchProcessor;
    CarRepository &repository;
    std::string backendName;
};

class CLICommand {
public:
    explicit CLICommand(std::string description)
        : description_(std::move(description)) {}
    virtual ~CLICommand() = default;
    const std::string &description() const { return description_; }
    virtual void execute(const std::vector<std::string> &args) = 0;

protected:
    std::string description_;
};

class CommandRegistry {
public:
    void add(std::string name, std::unique_ptr<CLICommand> command) {
        commands_.emplace(std::move(name), std::move(command));
    }

    CLICommand *find(const std::string &name) const {
        const auto it = commands_.find(name);
        if (it == commands_.end()) {
            return nullptr;
        }
        return it->second.get();
    }

    std::vector<std::pair<std::string, std::string>> descriptions() const {
        std::vector<std::pair<std::string, std::string>> list;
        for (const auto &pair : commands_) {
            list.emplace_back(pair.first, pair.second->description());
        }
        return list;
    }

private:
    std::map<std::string, std::unique_ptr<CLICommand>> commands_;
};

class ListCarsCommand : public CLICommand {
public:
    explicit ListCarsCommand(CommandContext &ctx)
        : CLICommand("List the top N available cars"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        size_t limit = 10;
        if (!args.empty()) {
            limit = static_cast<size_t>(std::stoul(args[0]));
        }

        const auto cars = ctx_.service.listAvailable(limit);
        if (cars.empty()) {
            std::cout << "No cars available." << std::endl;
            return;
        }

        for (const auto &car : cars) {
            std::cout << car.id << " (" << car.model << ") - condition "
                      << car.condition << ", price " << car.pricePerDay
                      << ", status: " << car.status << std::endl;
        }
    }

private:
    CommandContext &ctx_;
};

class RentCommand : public CLICommand {
public:
    explicit RentCommand(CommandContext &ctx)
        : CLICommand("Rent a car using Strategy pricing"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        if (args.size() < 2) {
            throw std::runtime_error("Usage: rent <carId> <userId>");
        }

        double amount = 0;
        if (ctx_.service.rentCar(args[0], args[1], amount)) {
            std::cout << "Car " << args[0] << " reserved. Amount due today: " << amount << " Rs." << std::endl;
        } else {
            std::cout << "Unable to rent car " << args[0] << std::endl;
        }
    }

private:
    CommandContext &ctx_;
};

class ReturnCommand : public CLICommand {
public:
    explicit ReturnCommand(CommandContext &ctx)
        : CLICommand("Return a car and close the transaction"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        if (args.size() != 1) {
            throw std::runtime_error("Usage: return <carId>");
        }

        if (ctx_.service.returnCar(args[0])) {
            std::cout << "Car " << args[0] << " returned successfully." << std::endl;
        } else {
            std::cout << "Unable to return car " << args[0] << std::endl;
        }
    }

private:
    CommandContext &ctx_;
};

class AddCarCommand : public CLICommand {
public:
    explicit AddCarCommand(CommandContext &ctx)
        : CLICommand("Add a new car to the repository"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        if (args.size() < 4) {
            throw std::runtime_error("Usage: add <carId> <model> <condition> <price>");
        }

        CarRecord record;
        record.id = args[0];
        record.model = args[1];
        record.condition = args[2];
        record.pricePerDay = std::stod(args[3]);

        ctx_.service.addCar(record);
        std::cout << "Car " << record.id << " added." << std::endl;
    }

private:
    CommandContext &ctx_;
};

class GenerateCommand : public CLICommand {
public:
    explicit GenerateCommand(CommandContext &ctx)
        : CLICommand("Generate synthetic fleet data"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        if (args.empty()) {
            throw std::runtime_error("Usage: generate <count> [outputFile]");
        }
        const size_t count = static_cast<size_t>(std::stoul(args[0]));
        const std::string output = args.size() > 1 ? args[1] : "synthetic_cars.txt";

        ctx_.generator.toFile(output, count);
        std::cout << "Generated " << count << " synthetic records in " << output << std::endl;
    }

private:
    CommandContext &ctx_;
};

class IngestCommand : public CLICommand {
public:
    explicit IngestCommand(CommandContext &ctx)
        : CLICommand("Ingest large car batches via buffered pipeline"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        if (args.empty()) {
            throw std::runtime_error("Usage: ingest <file> [chunkSize]");
        }
        const std::string file = args[0];
        const size_t chunk = args.size() > 1 ? static_cast<size_t>(std::stoul(args[1])) : 4096;

        auto metrics = ctx_.batchProcessor.ingest(file, chunk);
        std::cout << "Processed " << metrics.processedRecords << " records in "
                  << metrics.batches << " batches (" << metrics.duration.count() << " ms)." << std::endl;
    }

private:
    CommandContext &ctx_;
};

class SaveCommand : public CLICommand {
public:
    explicit SaveCommand(CommandContext &ctx)
        : CLICommand("Flush pending changes through transactional writer"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        (void)args;
        ctx_.service.save();
    }

private:
    CommandContext &ctx_;
};

class StatsCommand : public CLICommand {
public:
    explicit StatsCommand(CommandContext &ctx)
        : CLICommand("Show repository metrics"), ctx_(ctx) {}

    void execute(const std::vector<std::string> &args) override {
        (void)args;
        std::cout << "Tracked cars: " << ctx_.repository.totalRecords()
                  << ", pending writes: " << std::boolalpha << ctx_.repository.pendingChanges() << std::endl;
    }

private:
    CommandContext &ctx_;
};

class CarRentalCLI {
public:
    explicit CarRentalCLI(CommandContext ctx)
        : ctx_(ctx) {
        registerCommands();
    }

    void run() {
        std::cout << "=== Modular File Processing System ===" << std::endl;
        std::cout << "Type 'help' to list commands or 'exit' to quit." << std::endl;

        std::string line;
        while (true) {
            std::cout << "> " << std::flush;
            if (!std::getline(std::cin, line)) {
                break;
            }
            if (line == "exit") {
                break;
            }
            if (line == "help") {
                printHelp();
                continue;
            }
            if (line.empty()) {
                continue;
            }

            auto tokens = tokenize(line);
            const std::string commandName = tokens.front();
            tokens.erase(tokens.begin());

            CLICommand *command = registry_.find(commandName);
            if (!command) {
                std::cout << "Unknown command. Type 'help' for options." << std::endl;
                continue;
            }

            try {
                command->execute(tokens);
            } catch (const std::exception &ex) {
                std::cerr << "Error: " << ex.what() << std::endl;
            }
        }
    }

private:
    static std::vector<std::string> tokenize(const std::string &line) {
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void registerCommands() {
        registry_.add("list", std::make_unique<ListCarsCommand>(ctx_));
        registry_.add("rent", std::make_unique<RentCommand>(ctx_));
        registry_.add("return", std::make_unique<ReturnCommand>(ctx_));
        registry_.add("add", std::make_unique<AddCarCommand>(ctx_));
        registry_.add("generate", std::make_unique<GenerateCommand>(ctx_));
        registry_.add("ingest", std::make_unique<IngestCommand>(ctx_));
        registry_.add("save", std::make_unique<SaveCommand>(ctx_));
        registry_.add("stats", std::make_unique<StatsCommand>(ctx_));
    }

    void printHelp() const {
        std::cout << "Available commands:" << std::endl;
        for (const auto &[name, description] : registry_.descriptions()) {
            std::cout << "  " << name << " - " << description << std::endl;
        }
        std::cout << "  help - Show this list" << std::endl;
        std::cout << "  exit - Quit the CLI" << std::endl;
    }

    CommandContext ctx_;
    CommandRegistry registry_;
};

struct CliArguments {
    BackendType backend{BackendType::File};
    std::string carsFile{"cars.txt"};
    bool legacyMode{false};
};

[[maybe_unused]] static CliArguments parseArguments(int argc, char **argv) {
    CliArguments args;
    for (int i = 1; i < argc; ++i) {
        const std::string value = argv[i];
        if (value == "--backend=memory") {
            args.backend = BackendType::Memory;
        } else if (value == "--backend=file") {
            args.backend = BackendType::File;
        } else if (value.rfind("--cars=", 0) == 0) {
            args.carsFile = value.substr(7);
        } else if (value == "--legacy" || value == "--mode=legacy") {
            args.legacyMode = true;
        } else if (value == "--mode=modular") {
            args.legacyMode = false;
        }
    }
    return args;
}

} // namespace car_rental

#define LEGACY_ENTRY_POINT legacy_cli_entry_point
#define main LEGACY_ENTRY_POINT
#include "original.cpp"
#undef main

namespace legacy_car_rental {
inline int run() {
    return LEGACY_ENTRY_POINT();
}
} // namespace legacy_car_rental

#ifndef CAR_RENTAL_LIBRARY
int main(int argc, char **argv) {
    using namespace car_rental;

    auto validator = std::make_shared<CarRecordValidator>();
    const auto cliArgs = parseArguments(argc, argv);
    if (cliArgs.legacyMode) {
        std::cout << "Launching legacy interactive car rental system..." << std::endl;
        return legacy_car_rental::run();
    }

    auto backend = StorageBackendFactory::create(cliArgs.backend, cliArgs.carsFile, validator);
    CarRepository repository(backend);
    RentalService service(repository);
    SyntheticDatasetGenerator generator(validator);
    BatchProcessor processor(repository, validator);

    CommandContext ctx{service, generator, processor, repository, backend->name()};
    CarRentalCLI cli(std::move(ctx));
    cli.run();
    return 0;
}
#endif
