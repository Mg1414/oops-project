#include "../as.cpp"

#include <cassert>
#include <filesystem>
#include <iostream>

using namespace car_rental;

int main() {
    auto validator = std::make_shared<CarRecordValidator>();
    SyntheticDatasetGenerator generator(validator);

    namespace fs = std::filesystem;
    const fs::path dataset = fs::temp_directory_path() / "car_rental_integration.csv";
    generator.toFile(dataset.string(), 1000);

    auto backend = std::make_shared<FileStorageBackend>(dataset.string(), validator);
    CarRepository repository(backend);
    RentalService service(repository);
    BatchProcessor processor(repository, validator);

    const auto metrics = processor.ingest(dataset.string(), 256);
    assert(metrics.processedRecords == 1000);
    assert(metrics.batches > 0);

    auto cars = repository.all();
    assert(!cars.empty());

    double amount = 0;
    const bool rented = service.rentCar(cars.front().id, "integration-user", amount);
    assert(rented);
    assert(amount == cars.front().pricePerDay);

    assert(service.returnCar(cars.front().id));

    fs::remove(dataset);
    std::cout << "integration tests passed" << std::endl;
    return 0;
}
