#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <filesystem>

namespace legacy_file {

struct CarRecord {
    std::string model;
    std::string condition;
    double price{0.0};
    std::string availability{"Available"};
    std::string dueDate{"None"};
};

class BufferedInputStream {
public:
    explicit BufferedInputStream(const std::string &path, std::size_t size = 1 << 16)
        : buffer_(size), stream_(path) {
        if (stream_.is_open()) {
            stream_.rdbuf()->pubsetbuf(buffer_.data(), static_cast<std::streamsize>(buffer_.size()));
        }
    }

    std::ifstream &stream() { return stream_; }

private:
    std::vector<char> buffer_;
    std::ifstream stream_;
};

class TransactionalFileWriter {
public:
    explicit TransactionalFileWriter(std::string path)
        : path_(std::move(path)) {}

    void write(const std::vector<std::string> &lines) const {
        namespace fs = std::filesystem;
        fs::path target(path_);
        fs::path temp = target;
        temp += ".tmp";

        fs::create_directories(target.parent_path().empty() ? fs::current_path() : target.parent_path());
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Unable to open temporary file for transactional write");
        }

        std::vector<char> buffer(1 << 16);
        out.rdbuf()->pubsetbuf(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        for (const auto &line : lines) {
            out << line << '\n';
        }
        out.flush();
        out.close();

        fs::rename(temp, target);
    }

private:
    std::string path_;
};

inline bool isValidCondition(const std::string &condition) {
    static const std::vector<std::string> allowed{
        "excellent", "good", "fair", "minordamages", "majordamages"};
    return std::find(allowed.begin(), allowed.end(), condition) != allowed.end();
}

inline std::optional<CarRecord> parseCarLine(const std::string &line) {
    if (line.empty()) {
        return std::nullopt;
    }

    std::stringstream ss(line);
    std::vector<std::string> tokens;
    std::string token;
    while (std::getline(ss, token, ',')) {
        tokens.push_back(token);
    }

    if (tokens.size() < 3) {
        std::cerr << "[WARN] Skipping malformed car record: " << line << std::endl;
        return std::nullopt;
    }

    CarRecord record;
    record.model = tokens[0];
    record.condition = tokens[1];
    try {
        record.price = std::stod(tokens[2]);
    } catch (const std::exception &) {
        std::cerr << "[WARN] Invalid price in car record: " << line << std::endl;
        return std::nullopt;
    }
    if (tokens.size() > 3) {
        record.availability = tokens[3];
    }
    if (tokens.size() > 4) {
        record.dueDate = tokens[4];
    }
    if (record.availability.empty()) {
        record.availability = "Available";
    }
    if (record.dueDate.empty()) {
        record.dueDate = "None";
    }

    if (!isValidCondition(record.condition) || record.price <= 0) {
        std::cerr << "[WARN] Skipping invalid car record: " << line << std::endl;
        return std::nullopt;
    }

    return record;
}

inline std::string serialize(const CarRecord &record) {
    std::ostringstream oss;
    oss << record.model << ',' << record.condition << ',' << record.price << ','
        << record.availability << ',' << record.dueDate;
    return oss.str();
}

class CarFilePipeline {
public:
    explicit CarFilePipeline(std::string path)
        : path_(std::move(path)), writer_(path_) {}

    std::vector<CarRecord> readAll() const {
        std::vector<CarRecord> records;
        BufferedInputStream input(path_);
        auto &stream = input.stream();
        if (!stream.is_open()) {
            return records;
        }
        std::string line;
        while (std::getline(stream, line)) {
            if (auto record = parseCarLine(line)) {
                records.push_back(*record);
            }
        }
        return records;
    }

    void writeAll(const std::vector<CarRecord> &records) const {
        std::vector<std::string> lines;
        lines.reserve(records.size());
        for (const auto &record : records) {
            lines.push_back(serialize(record));
        }
        writer_.write(lines);
    }

private:
    std::string path_;
    TransactionalFileWriter writer_;
};

inline CarFilePipeline &carPipeline() {
    static CarFilePipeline pipeline("cars.txt");
    return pipeline;
}

inline std::vector<CarRecord> loadCars() {
    return carPipeline().readAll();
}

inline void saveCars(const std::vector<CarRecord> &records) {
    carPipeline().writeAll(records);
}

inline bool mutateCars(const std::function<bool(std::vector<CarRecord> &)> &fn) {
    auto cars = loadCars();
    const bool changed = fn(cars);
    if (changed) {
        saveCars(cars);
    }
    return changed;
}

inline CarRecord makeRecord(const std::string &model, const std::string &condition, double price) {
    CarRecord record;
    record.model = model;
    record.condition = condition;
    record.price = price;
    record.availability = "Available";
    record.dueDate = "None";
    return record;
}

} // namespace legacy_file

using namespace std;
class User
{


public:

    
    void add_Manager(){
        string name;
        string id;
        string password;

        cout << "Enter the Name: ";
        cin >> name;

        cout << "Enter the ID: ";
        cin >> id;

        cout << "Enter the Password: ";
        cin >> password;

        ofstream fout("managers.txt", ios::app); // Open the file in append mode

        fout << name << "," << id << "," << password << endl; // Write data to the file
        fout.close();

        cout << "manager"<<" added successfully." << endl;
    }
    void add_Customer()
    {
        string name;
        string id;
        string password;

        cout << "Enter the Name: ";
        cin >> name;

        cout << "Enter the ID: ";
        cin >> id;

        cout << "Enter the Password: ";
        cin >> password;

        ofstream fout("customers.txt", ios::app); // Open the file in append mode

        fout << name << "," << id << "," << password << endl; // Write data to the file
        fout.close();

        cout << "Customer added successfully." << endl;
    }
    void add_Employee()
    {
        std::string name;
        std::string id;
        std::string password;

        std::cout << "Enter the Name: ";
        std::getline(std::cin >> std::ws, name);

        std::cout << "Enter the ID: ";
        std::getline(std::cin, id);

        std::cout << "Enter the Password: ";
        std::getline(std::cin, password);

        std::ofstream fout("employees.txt", std::ios::app); // Open the file in append mode

        fout << name << "," << id << "," << password << ",B" << std::endl; // Write data to the file
        fout.close();

        std::cout << "Employee added successfully." << std::endl;
    }
    void showavailablecars(const string&role)
    {
        auto cars = legacy_file::loadCars();
        if (cars.empty())
        {
            cout << "No cars present in the inventory." << endl;
            return;
        }

        cout << "Available Cars:" << endl;
        for (const auto &car : cars)
        {
            if (car.availability != "Available")
            {
                continue;
            }

            double price = car.price;
            if(role=="employee"){
                price *= 0.85;
            }
            cout << car.model <<","<<car.condition<<","<< price <<"Rs.\n"<< endl;
        }
    }

    // Other common functionalities for users
};

class Manager : public User
{
public:
    void verifyManagers(int iD, const string &password)
    {
        ifstream file("managers.txt");
        if (file.is_open())
        {
            string line;
            string id = to_string(iD);
            while (getline(file, line))
            {
                istringstream iss(line);
                string name, ID, PASSWORD;

                getline(iss, name, ',');
                getline(iss, ID, ',');
                getline(iss, PASSWORD, ',');

                if (id == ID && password == PASSWORD)
                {
                    cout << "Name: " << name << endl;
                    // Store rented cars as a vector

                    managerFunctionality(name);
                    return;
                }
            }
            cout << "Invalid" << endl;
            file.close();
        }
        else
        {
            cerr << "Error: Unable to open file." << endl;
        }
    }
    void addCar()
    {
        string model;
        string condition;
        double price;

        cout << "Enter the Model: ";
        cin >> model;

        cout << "Enter the Condition: ";
        cin >> condition;

        cout << "Enter the Price: ";
        cin >> price;

        // Check if the input operation for price was successful
        if (cin.fail())
        {
            cout << "Invalid input for price. Please enter a valid number." << endl;
            cin.clear();                                         // Clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Discard invalid input
            return;
        }

        if (!legacy_file::isValidCondition(condition))
        {
            cout << "Invalid condition. Allowed values: excellent, good, fair, minordamages, majordamages." << endl;
            return;
        }

        bool added = legacy_file::mutateCars([&](std::vector<legacy_file::CarRecord> &cars) {
            auto exists = std::find_if(cars.begin(), cars.end(), [&](const legacy_file::CarRecord &record) {
                return record.model == model;
            });
            if (exists != cars.end())
            {
                return false;
            }
            cars.push_back(legacy_file::makeRecord(model, condition, price));
            return true;
        });

        if (added)
        {
            cout << "Car added successfully." << endl;
        }
        else
        {
            cout << "Car with the same model already exists." << endl;
        }
    }


void deleteCar(const std::string &modelToDelete) {
    bool deleted = false;
    legacy_file::mutateCars([&](std::vector<legacy_file::CarRecord> &cars) {
        auto oldSize = cars.size();
        cars.erase(std::remove_if(cars.begin(), cars.end(), [&](const legacy_file::CarRecord &record) {
            return record.model == modelToDelete;
        }), cars.end());
        deleted = cars.size() != oldSize;
        return deleted;
    });

    if (deleted) {
        std::cout << "Car deleted successfully." << std::endl;
    } else {
        std::cout << "Car not found." << std::endl;
    }
}


    void searchCar(const std::string &searchCriteria)
    {
        auto cars = legacy_file::loadCars();
        for (const auto &car : cars)
        {
            const std::string priceStr = std::to_string(static_cast<int>(car.price));
            if (car.model == searchCriteria || car.condition == searchCriteria || priceStr == searchCriteria)
            {
                std::cout << "Model: " << car.model << ", Condition: " << car.condition << ", Price: " << car.price << std::endl;
            }
        }
    }
 

    void updateCar(const std::string &modelToUpdate)
    {
        bool updated = false;
        legacy_file::mutateCars([&](std::vector<legacy_file::CarRecord> &cars) {
            for (auto &car : cars)
            {
                if (car.model == modelToUpdate)
                {
                    std::string newModel;
                    std::string newCondition;
                    double newPrice;

                    std::cout << "Enter new model for car " << car.model << ": ";
                    std::cin >> newModel;

                    std::cout << "Enter new condition for car " << car.model << ": ";
                    std::cin >> newCondition;

                    std::cout << "Enter new price for car " << car.model << ": ";
                    std::cin >> newPrice;

                    if (std::cin.fail())
                    {
                        std::cerr << "Invalid input for price. Please enter a valid number." << std::endl;
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        return false;
                    }

                    if (!legacy_file::isValidCondition(newCondition))
                    {
                        std::cerr << "Invalid condition." << std::endl;
                        return false;
                    }

                    car.model = newModel;
                    car.condition = newCondition;
                    car.price = newPrice;
                    updated = true;
                    return true;
                }
            }
            return false;
        });

        if (updated)
        {
            std::cout << "Car details updated successfully." << std::endl;
        }
        else
        {
            std::cout << "Car not found." << std::endl;
        }
    }

    //////////////////
    void addCustomer()
    {
        string name;
        string id;
        string password;

        cout << "Enter the Name: ";
        cin >> name;

        cout << "Enter the ID: ";
        cin >> id;

        cout << "Enter the Password: ";
        cin >> password;

        ofstream fout("customers.txt", ios::app); // Open the file in append mode

        fout << name << "," << id << "," << password << endl; // Write data to the file
        fout.close();

        cout << "Customer added successfully." << endl;
    }

    void deleteCustomer(const string &idToDelete)
    {
        ifstream inFile("customers.txt");
        ofstream outFile("temp.txt");

        if (!inFile)
        {
            cerr << "Unable to open file." << endl;
            return;
        }

        string line;
        bool found = false;
        while (getline(inFile, line))
        {
            if (line.substr(line.find(',') + 1, line.find(',', line.find(',') + 1) - line.find(',') - 1) != idToDelete)
            {
                outFile << line << endl;
            }
            else
            {
                found = true;
            }
        }

        inFile.close();
        outFile.close();

        remove("customers.txt");
        rename("temp.txt", "customers.txt");

        if (found)
        {
            cout << "Customer deleted successfully." << endl;
        }
        else
        {
            cout << "Customer not found." << endl;
        }
    }

    void searchCustomer(const std::string &searchCriteria)
    {
        std::ifstream file("customers.txt");
        if (file.is_open())
        {
            std::string line;
            while (getline(file, line))
            {
                std::istringstream iss(line);
                std::string name, id, password;

                // Parse customer details from the line
                getline(iss, name, ',');
                getline(iss, id, ',');
                getline(iss, password, ',');

                // Check if customer matches search criteria
                if ( id == searchCriteria)
                {
                    // Display customer details
                    std::cout << "Name: " << name << ", ID: " << id << std::endl;
                }
            }
            file.close();
        }
        else
        {
            std::cerr << "Error: Unable to open file." << std::endl;
        }
    }

    void updateCustomer(const std::string &idToUpdate)
    {
        std::ifstream inFile("customers.txt");
        std::ofstream outFile("temp.txt");

        if (!inFile)
        {
            std::cerr << "Unable to open file." << std::endl;
            return;
        }

        std::string line;
        bool found = false;
        while (std::getline(inFile, line))
        {
            std::string name, id, password, rating;

            std::istringstream iss(line);

            // Parse employee details from the line
            std::getline(iss, name, ',');
            std::getline(iss, id, ',');
            std::getline(iss, password, ',');

           
            std::getline(iss, rating, ',');

            if (id == idToUpdate)
            {
                // Prompt user for new details
                std::string newName;
                std::string newPassword;
               
                string newRating;

                // Clear input buffer
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::cout << "Enter new name for customer with ID " << id << ": ";
                std::getline(std::cin, newName); // Read name with spaces
                std::cout << "Enter new password for customer with ID " << id << ": ";
                std::getline(std::cin, newPassword); // Read password
               
                std::cout << "Enter new rating for customer with ID " << id << ": ";
                std::cin >> newRating; // Read rating

                // Write updated details to temporary file
                outFile << newName << "," << id << "," << newPassword << ",";
                outFile  << newRating << std::endl; // Write new fine and rating
                found = true;
            }
            else
            {
                // Write existing details to temporary file
                outFile << line << std::endl;
            }
        }

        inFile.close();
        outFile.close();

        // Replace original file with temporary file
        std::remove("customers.txt");
        std::rename("temp.txt", "customers.txt");

        if (found)
        {
            std::cout << "Customer details updated successfully." << std::endl;
        }
        else
        {
            std::cout << "Customer not found." << std::endl;
        }
    }
    
    //////////
    void addEmployee()
    {
        std::string name;
        std::string id;
        std::string password;

        std::cout << "Enter the Name: ";
        std::getline(std::cin >> std::ws, name);

        std::cout << "Enter the ID: ";
        std::getline(std::cin, id);

        std::cout << "Enter the Password: ";
        std::getline(std::cin, password);

        std::ofstream fout("employees.txt", std::ios::app); // Open the file in append mode

        fout << name << "," << id << "," << password << ",B" << std::endl; // Write data to the file
        fout.close();

        std::cout << "Employee added successfully." << std::endl;
    }

    void deleteEmployee(const string &idToDelete)
    {
        ifstream inFile("employees.txt");
        ofstream outFile("temp.txt");

        if (!inFile)
        {
            cerr << "Unable to open file." << endl;
            return;
        }

        string line;
        bool found = false;
        while (getline(inFile, line))
        {
            if (line.substr(line.find(',') + 1, line.find(',', line.find(',') + 1) - line.find(',') - 1) != idToDelete)
            {
                outFile << line << endl;
            }
            else
            {
                found = true;
            }
        }

        inFile.close();
        outFile.close();

        remove("employees.txt");
        rename("temp.txt", "employees.txt");

        if (found)
        {
            cout << "Employee deleted successfully." << endl;
        }
        else
        {
            cout << "Employee not found." << endl;
        }
    }

    void searchEmployee(const string &searchCriteria)
    {
        std::ifstream file("employees.txt");
        if (file.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                std::istringstream iss(line);
                std::string name, id, password;

                // Parse employee details from the line
                std::getline(iss, name, ',');
                std::getline(iss, id, ',');
                std::getline(iss, password, ',');

                // Check if employee matches search criteria
                if ( id == searchCriteria)
                {
                    // Display employee details
                    cout << "Name: " << name << ", ID: " << id << endl;
                }
            }
            file.close();
        }
        else
        {
            std::cerr << "Error: Unable to open file." << std::endl;
        }
    }

    void updateEmployee(const std::string &idToUpdate)
    {
        std::ifstream inFile("employees.txt");
        std::ofstream outFile("temp.txt");

        if (!inFile)
        {
            std::cerr << "Unable to open file." << std::endl;
            return;
        }

        std::string line;
        bool found = false;
        while (std::getline(inFile, line))
        {
            std::string name, id, password, rating;

            std::istringstream iss(line);

            // Parse employee details from the line
            std::getline(iss, name, ',');
            std::getline(iss, id, ',');
            std::getline(iss, password, ',');

           
            std::getline(iss, rating, ',');

            if (id == idToUpdate)
            {
                // Prompt user for new details
                std::string newName;
                std::string newPassword;
               
                string newRating;

                // Clear input buffer
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                std::cout << "Enter new name for employee with ID " << id << ": ";
                std::getline(std::cin, newName); // Read name with spaces
                std::cout << "Enter new password for employee with ID " << id << ": ";
                std::getline(std::cin, newPassword); // Read password
               
                std::cout << "Enter new rating for employee with ID " << id << ": ";
                std::cin >> newRating; // Read rating

                // Write updated details to temporary file
                outFile << newName << "," << id << "," << newPassword << ",";
                outFile  << newRating << std::endl; // Write new fine and rating
                found = true;
            }
            else
            {
                // Write existing details to temporary file
                outFile << line << std::endl;
            }
        }

        inFile.close();
        outFile.close();

        // Replace original file with temporary file
        std::remove("employees.txt");
        std::rename("temp.txt", "employees.txt");

        if (found)
        {
            std::cout << "Employee details updated successfully." << std::endl;
        }
        else
        {
            std::cout << "Employee not found." << std::endl;
        }
    }

   void showallCars()
{
    auto cars = legacy_file::loadCars();
    if (cars.empty())
    {
        std::cerr << "Error: Unable to open file or no cars present." << std::endl;
        return;
    }

    std::cout << "Contents of cars.txt:" << std::endl;
    for (const auto &car : cars)
    {
        std::cout << legacy_file::serialize(car) << std::endl;
    }
}

private:
   void managerFunctionality(const string &name)
{
    bool loggedin = true;
    while (loggedin)
    {
        cout<<"\n\n\n";
        cout << "Welcome Manager " << name << endl;
        cout << "Select a category of operation:" << endl;
        cout << "1. Manage cars" << endl;
        cout << "2. Manage customers" << endl;
        cout << "3. Manage employees" << endl;
        cout << "4. View all the cars" << endl;
        cout << "5. Logout" << endl;
        int choose;
        cin >> choose;

        switch (choose)
        {
        case 1:
        {
            cout << "Select an operation:" << endl;
            cout << "1. Add" << endl;
            cout << "2. Update" << endl;
            cout << "3. Search" << endl;
            cout << "4. Delete" << endl;
            cout << "5. Go back to Home View" << endl;
            int operation;
            cin >> operation;

            switch (operation)
            {
            case 1:{
                addCar();
                break;
            }
            case 2:
            {
                string modelToUpdate;
                cout << "Enter the car model";
                cin >> modelToUpdate;
                updateCar(modelToUpdate);
                break;
            }
            case 3:
            {
                string searchCriteria;
                cout << "Enter the car model or car condition";
                cin >> searchCriteria;
                searchCar(searchCriteria);
                break;
            }
            case 4:
            {
                string modelToDelete;
                cout << "Enter the car model";
                cin >> modelToDelete;
                deleteCar(modelToDelete);
                break;
            }
            case 5:
            {
            break;
            }
            default:
                cout << "Invalid operation." << endl;
                break;
            }
            

            break;
        }

        case 2:
        {
            cout << "Select an operation:" << endl;
            cout << "1. Add" << endl;
            cout << "2. Update" << endl;
            cout << "3. Search" << endl;
            cout << "4. Delete" << endl;
            cout << "5. Go back to Home View" << endl;
            int operation;
            cin >> operation;

            switch (operation)
            {
            case 1:{
                addCustomer();
                break;
            }
            case 2:
            {
                string idToUpdate;
                cout << "Enter the customer ID";
                cin >> idToUpdate;
                updateCustomer(idToUpdate);
                break;
            }
            case 3:
            {
                string searchCriteria;
                cout << "Enter the customer ID ";
                cin >> searchCriteria;
                searchCustomer(searchCriteria);
                break;
            }
            case 4:
            {
                string idToDelete;
                cout << "Enter the customer ID";
                cin >> idToDelete;
                deleteCustomer(idToDelete);
                break;
            }
            case 5:{
            break;
            }
            default:
                break;
            }


            break;
        }

        case 3:
        {
            cout << "Select an operation:" << endl;
            cout << "1. Add" << endl;
            cout << "2. Update" << endl;
            cout << "3. Search" << endl;
            cout << "4. Delete" << endl;
            cout << "5. Go back to Home View" << endl;
            int operation;
            cin >> operation;

            switch (operation)
            {
            case 1:{
                addEmployee();
                break;
            }
            case 2:
            {
                string idToUpdate;
                cout << "Enter the employee ID:";
                cin >> idToUpdate;
                updateEmployee(idToUpdate);
                break;
            }
            case 3:
            {
                string searchCriteria;
                cout << "Enter the employee ID :";
                cin >> searchCriteria;
                searchEmployee(searchCriteria);
                break;
            }
            case 4:
            {
                string idToDelete;
                cout << "Enter the employee ID: ";
                cin >> idToDelete;
                deleteEmployee(idToDelete);
                break;
            }
            case 5:{
            break;
            }
            default:
                break;
            }

            break;
        }

        case 4:
            // View all cars
            showallCars();
            break;

        case 5:
            cout << "Logged Out" << endl;
            loggedin = false; // Set loggedin to false to exit the loop
            break;

        default:
            cout << "Invalid choice." << endl;
            break;
        }
    }
}

};

class Customer_Employee: public User{
    public:

    

int myrentedCars(int id)
{
    int rented = 0;
    auto cars = legacy_file::loadCars();
    bool Found = false;
    std::vector<std::pair<std::string, std::string>> rentedCars; // Store model and due date as pairs

    for (const auto &car : cars)
    {
        if (car.availability == "Rented by the user ID: " + std::to_string(id))
        {
            rentedCars.push_back(std::make_pair(car.model, car.dueDate));
            rented++;
            Found = true;
        }
    }

    if (Found)
    {
        std::cout << "Rented cars for the user with ID " << id << ":\n";
        for (const auto &car : rentedCars)
        {
            std::cout << "Model: " << car.first << ", Due Date: " << car.second << std::endl;
        }
    }
    else
    {
        std::cout << "The user with ID " << id << " has not rented any cars." << std::endl;
    }
    return rented;
}

int rentable_cars(const string& role, int Id)
{
    int limit = 0;
    bool found = false;

    // Open user file to find the rating and limit based on rating
    ifstream userFile(role + "s.txt");
    if (!userFile) {
        cerr << "Unable to open employee file." << endl;
        return 0;
    }

    string line;
    while (getline(userFile, line)) {
        stringstream ss(line);
        string name, password, rating;
        int userId; // Change variable name to avoid conflict with function argument
        string ID;

        getline(ss, name, ',');
        getline(ss, ID, ',');
        userId = stoi(ID); // Convert ID string to int
        getline(ss, password, ',');
      
        ss >> rating; // Correct way to read rating

        // Determine the limit based on rating
        if (rating == "A+" || rating == "A") {
            limit = 4;
        } else if (rating == "B+" || rating == "B") {
            limit = 3;
        } else if (rating == "C+" || rating == "C") {
            limit = 2;
        } else if (rating == "D+" || rating == "D") {
            limit = 1;
        } else {
            limit = 0; // Handle invalid rating
        }

        // Check if the current line corresponds to the user ID
        if (userId == Id) {
            found = true;
            break; // No need to continue reading lines once user is found
        }
    }
    userFile.close();

    // Find the number of cars already rented by the id from cars.txt
    int rented = 0;
    auto cars = legacy_file::loadCars();
    for (const auto &car : cars) {
        if (car.availability == "Rented by the user ID: " + to_string(Id)) {
            found = true;
            rented++;
        }
    }

    int rentingLimit = limit - rented;

    // Print how many more cars the user can rent
    if (found) {
        if (rentingLimit > 0) {
            cout << "You can rent " << rentingLimit << " more car(s)." << endl;
            showavailablecars(role);
        } else {
            cout << "You can't rent anymore." << endl;
        }
        return rentingLimit;
    } else {
        cout << "User with ID " << Id << " not found." << endl;
        return 0;
    }
}
void rent_request(const string& role, int id)
{
    string carModel;
    cout << "Enter the car model: ";
    cin >> carModel;

    int rentingLimit = rentable_cars(role, id);

    if (rentingLimit <= 0) {
        return;
    }

    bool found = false;
    bool rentable = false;
    double amountToPay = 0.0;

    legacy_file::mutateCars([&](std::vector<legacy_file::CarRecord> &cars) {
        for (auto &car : cars) {
            if (car.model == carModel) {
                found = true;
                if (car.availability == "Available") {
                    rentable = true;
                    time_t now = time(0);
                    tm *nowPlus30Days = localtime(&now);
                    nowPlus30Days->tm_mday += 30;
                    mktime(nowPlus30Days);
                    char buffer[80];
                    strftime(buffer, 80, "%Y-%m-%d", nowPlus30Days);
                    car.availability = "Rented by the user ID: " + std::to_string(id);
                    car.dueDate = buffer;
                    amountToPay = car.price;
                    return true;
                }
                return false;
            }
        }
        return false;
    });

    if (!found) {
        cout << "Car not found." << endl;
    } else if (!rentable) {
        cout << "Car is not available." << endl;
    } else {
        if(role=="customer"){
            cout<<"Please pay"<<amountToPay<<"Rs."<<endl;
        }
        else{
            double discounted = amountToPay * 0.85;
            cout<<"Please pay"<<discounted<<"Rs."<<endl;
        }
        cout<<"Processing your payment..."<<endl;
        cout << "Car rented successfully." <<endl;
    }
}






void show_fine(const std::string& role, int id) {
    auto cars = legacy_file::loadCars();
    bool found = false;
    int Fine = 0;

    for (const auto &car : cars) {
        if (car.availability == "Rented by the user ID: " + std::to_string(id)) {
            found = true;

            time_t now = time(0); // Current time
            std::istringstream iss(car.dueDate);
            int year, month, day;
            char delimiter;
            if (!(iss >> year >> delimiter >> month >> delimiter >> day)) {
                continue;
            }

            tm dueDate = {};
            dueDate.tm_year = year - 1900;
            dueDate.tm_mon = month - 1;
            dueDate.tm_mday = day;

            time_t dueTime = mktime(&dueDate);
            double differenceSeconds = difftime(now, dueTime);
            int lateDays = differenceSeconds / (60 * 60 * 24); // Calculate late days

            if (lateDays > 0) {
                Fine += 20 * lateDays; // Calculate additional fine
            }
        }
    }

    if (found) {
        std::cout << "Total fine for " << role << " with ID " << id << " is: " << Fine << std::endl;
    } else {
         std::cout << "Total fine for " << role << " with ID " << id << " is: " << "0" << std::endl;
    }
}
void show_fine_car(const string& carModel,const std::string& role, int id){
    auto cars = legacy_file::loadCars();
    bool found = false;
    int Fine = 0;

    for (const auto &car : cars) {
        if (car.model == carModel && car.availability == "Rented by the user ID: " + std::to_string(id)) {
            found = true;

            time_t now = time(0); // Current time
            std::istringstream iss(car.dueDate);
            int year, month, day;
            char delimiter;
            if (!(iss >> year >> delimiter >> month >> delimiter >> day)) {
                continue;
            }

            tm dueDate = {};
            dueDate.tm_year = year - 1900;
            dueDate.tm_mon = month - 1;
            dueDate.tm_mday = day;

            time_t dueTime = mktime(&dueDate);
            double differenceSeconds = difftime(now, dueTime);
            int lateDays = differenceSeconds / (60 * 60 * 24); // Calculate late days

            if (lateDays > 0) {
                Fine += 20 * lateDays; // Calculate additional fine
            }
        }
    }

    if (found) {
        std::cout << "The fine for  " << carModel  << " is: " << Fine << std::endl;
        cout<<"Please pay the above amount."<<endl;
    } else {
         std::cout << "The car was not rented by the " << role << " with ID " << id  << std::endl;
    }
}
void return_request(int id, const std::string& role) {
    switch(myrentedCars(id)){
    case 0:
    {
        cout<<"You have no rented cars.";
        return;
    }
    case -1:
    {
        std::cerr << "Unable to open file." << std::endl;
        return;
    }
    default:
    {
    break;
    }
}
    
    std::string carModel;

    std::cout << "Enter the car model: ";
    std::cin >> carModel;

    // Show fine for the returned car
    show_fine_car(carModel, role, id);

    bool found = false;
    std::string choice;
    std::string newCondition;

    bool updated = legacy_file::mutateCars([&](std::vector<legacy_file::CarRecord> &cars) {
        for (auto &car : cars) {
            if (car.model == carModel && car.availability == "Rented by the user ID: " + std::to_string(id)) {
                found = true;
                std::cout << "Enter the new condition of the car:\n"
                             "1. excellent\n"
                             "2. good\n"
                             "3. fair\n"
                             "4. minordamages\n"
                             "5. majordamages\n";
                std::cin >> choice;

                if (choice == "1") {
                    newCondition = "excellent";
                } else if (choice == "2") {
                    newCondition = "good";
                } else if (choice == "3") {
                    newCondition = "fair";
                } else if (choice == "4") {
                    newCondition = "minordamages";
                } else if (choice == "5") {
                    newCondition = "majordamages";
                } else {
                    std::cerr << "Invalid condition choice." << std::endl;
                    return false;
                }

                car.condition = newCondition;
                car.availability = "Available";
                car.dueDate = "None";
                return true;
            }
        }
        return false;
    });

    if (!found) {
        std::cout << "Car with model " << carModel << " not found or not rented by user ID " << id << std::endl;
        return;
    }

    if (!updated) {
        return;
    }

    std::string ratingFile = role + "s.txt";
    std::ifstream userFile(ratingFile);
    std::ofstream tempUserFile("temp_emp.txt");

    if (!userFile || !tempUserFile) {
        std::cerr << "Unable to open user file." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(userFile, line)) {
        std::stringstream ss(line);
        std::string name, Id, password, rating;

        std::getline(ss, name, ',');
        std::getline(ss, Id, ',');
        std::getline(ss, password, ',');
        std::getline(ss, rating);

        if (std::stoi(Id) == id) {
            int ratingValue;

            if (rating == "A") {
                ratingValue = 8;
            } else if (rating == "A+") {
                ratingValue = 9;
            } else if (rating == "B+") {
                ratingValue = 7;
            } else if (rating == "B") {
                ratingValue = 6;
            } else if (rating == "C+") {
                ratingValue = 5;
            } else if (rating == "C") {
                ratingValue = 4;
            } else if (rating == "D+") {
                ratingValue = 3;
            } else if (rating == "D") {
                ratingValue = 2;
            } else {
                std::cerr << "Invalid rating: " << rating << std::endl;
                return;
            }

            if (choice == "1") {
                ratingValue += 2;
            } else if (choice == "4" || choice == "5") {
                ratingValue -= 1;
            }

            ratingValue = std::max(2, std::min(9, ratingValue));

            std::string newRating;
            switch (ratingValue) {
                case 8:
                    newRating = "A";
                    break;
                case 9:
                    newRating = "A+";
                    break;
                case 7:
                    newRating = "B+";
                    break;
                case 6:
                    newRating = "B";
                    break;
                case 5:
                    newRating = "C+";
                    break;
                case 4:
                    newRating = "C";
                    break;
                case 3:
                    newRating = "D+";
                    break;
                case 2:
                    newRating = "D";
                    break;
                default:
                    std::cerr << "Invalid rating value: " << ratingValue << std::endl;
                    return;
            }

            tempUserFile << name << "," << Id << "," << password << "," << newRating << std::endl;
        } else {
            tempUserFile << line << std::endl;
        }
    }

    userFile.close();
    tempUserFile.close();

    std::remove(ratingFile.c_str());
    std::rename("temp_emp.txt", ratingFile.c_str());

    std::cout << "Car returned successfully." << std::endl;
}

    void view_profile(int id,const string& role)
    {
        ifstream inFile(role+"s.txt");

        if (!inFile)
        {
            cerr << "Unable to open file." << endl;
            return;
        }

        string line;
        bool found = false;
        while (getline(inFile, line))
        {
            stringstream ss(line);
            string name, employeeID, password, fine, rating;

            getline(ss, name, ',');
            getline(ss, employeeID, ',');
            getline(ss, password, ',');

            getline(ss, rating, ',');

            if (stoi(employeeID) == id)
            {
                found = true;
                cout << "Name: " << name << ", ID: " << employeeID << ", Password: " << password <<  ", Rating: " << rating << endl;

                break;
            }
        }

        if (!found)
        {
            cerr << "The user not found." << endl;
        }

        inFile.close();
    }


};

class Customer : public Customer_Employee
{
public:
void verify_customer(int iD, const string &password)
    {
        ifstream file("customers.txt");
        if (file.is_open())
        {
            string line;
            string str_id = to_string(iD);
            while (getline(file, line))
            {
                istringstream iss(line);
                
                std::stringstream ss(line);
                std::string name, Id, PASSWORD, fine, rating;

                std::getline(ss, name, ',');
                std::getline(ss, Id, ',');
                std::getline(ss, PASSWORD, ',');
                
                std::getline(ss, rating);
                

                if (str_id == Id && password == PASSWORD)
                {
                   
                        cout << "Name: " << name << endl;
                    customerFunctionality(iD);
                    return;
                    
                }
            }
            cout << "Invalid" << endl;
            file.close();
        }
        else
        {
            cerr << "Error: Unable to open file." << endl;
        }
    }
   
private:
    void customerFunctionality(int id)
{
    bool loggedIn = true;

    while (loggedIn)
    {
        cout<<"\n";
        cout << "Welcome! You can perform your tasks here." << endl;
        cout << "Select an option:" << endl;
        cout << "1. Show the available cars" << endl;
        cout << "2. View your rented cars" << endl;
        cout << "3. View customer profile" << endl;
        cout << "4. Request to rent a car" << endl;
        cout << "5. Request to return a car" << endl;
        cout << "6. View your fine" << endl;
        cout << "7. Logout" << endl;
        cout << "Enter your choice: ";

        int choose;
        cin >> choose;

        switch (choose)
        {
        case 1:
        {
            showavailablecars("customer");
            break;
        }
        case 2:
        {
            myrentedCars(id);
            break;
        }
        case 3:
        {
            view_profile(id, "customer");
            break;
        }
        case 4:
        {
            if (rentable_cars("customer", id) > 0)
                rent_request("customer", id);
            break;
        }
        case 5:
        {
            return_request(id, "customer");
            break;
        }
        case 6:
        {
            show_fine("customer", id);
            break;
        }
        case 7:
        {
             cout << "Logged Out" << endl;
        cout<<"\n";
            loggedIn = false;
            break;
        }
        default:
            cout << "Invalid choice." << endl;
            break;
        }
    }
}

    
};

class Employee : public Customer_Employee
{
public:
    void verify_employee(int iD, const string &password)
    {
        ifstream file("employees.txt");
        if (file.is_open())
        {
            string line;
            string str_id = to_string(iD);
            while (getline(file, line))
            {
                istringstream iss(line);
                vector<string> rentedCars;
                string name, ID, PASSWORD, rating;

                getline(iss, name, ',');
                getline(iss, ID, ',');
                getline(iss, PASSWORD, ',');
               
                getline(iss, rating, ',');

                if (str_id == ID && password == PASSWORD)
                {
                   
                    cout << "Name: " << name << endl;
                    employeeFunctionality(iD);
                    return;
                    
                }
            }
            cout << "Invalid" << endl;
            file.close();
        }
        else
        {
            cerr << "Error: Unable to open file." << endl;
        }
    }
private:
    void employeeFunctionality(int id)
{
    bool loggedIn = true;

    while (loggedIn)
    {
        cout<<"\n";
        cout << "Welcome! You can perform your tasks here." << endl;
        cout << "Select an option:" << endl;
        cout << "1. Show the available cars" << endl;
        cout << "2. View your rented cars" << endl;
        cout << "3. View employee profile" << endl;
        cout << "4. Request to rent a car" << endl;
        cout << "5. Request to return a car" << endl;
        cout << "6. View your fine" << endl;
        cout << "7. Logout" << endl;
        cout << "Enter your choice: ";

        int choose;
        cin >> choose;

        switch (choose)
        {
        case 1:
            showavailablecars("employee");
            break;
        case 2:
        {
            myrentedCars(id);
            break;
        }
        case 3:
        {
            view_profile(id, "employee");
            break;
        }
        case 4:
        {
            if (rentable_cars("employee", id) > 0)
                rent_request("employee", id);
            break;
        }
        case 5:
        {
            
            return_request( id, "employee");

            break;
        }
        case 6:
        {
            show_fine( "employee",id);
            
            break;
        }
        case 7:
        cout << "Logged Out" << endl;
        cout<<"\n";
            loggedIn = false;
            break;
        default:
            cout << "Invalid choice." << endl;
            break;
        }
    }
    
}

};


void mainFunction();

int main()
{
    while (true)
    {
      
        mainFunction(); // Run the main functionality
    }

    return 0;
}



void mainFunction()
{
    
    cout << "Select your role:" << endl;
    cout << "1. Login as a Manager" << endl;
    cout << "2. Login as a Customer" << endl;
    cout << "3. Login as an Employee" << endl;
    cout << "4. Register as a new manager" << endl;
    cout << "5. Register as a new customer" << endl;
    cout << "6. Register as a new employee" << endl;
    cout << "7. How to use car rental system" << endl;
    cout << "8. Exit car rental system" << endl;
    User user;

    int roleChoice;
    cout << "Enter your choice (1-8): ";
    cin >> roleChoice;

    string role;
    
    int id;
    string password;
    
    switch (roleChoice)
    {
    case 1:
        role = "manager";
        cout << "Enter your ID: ";
        cin >> id;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid ID. Please enter a numeric ID." << endl;
            return;
        }
        cout << "Enter your password: ";
        cin >> password;
        break;
    case 2:
        role = "customer";
        cout << "Enter your ID: ";
        cin >> id;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid ID. Please enter a numeric ID." << endl;
            return;
        }
        cout << "Enter your password: ";
        cin >> password;
        break;
    case 3:
        role = "employee";
        cout << "Enter your ID: ";
        cin >> id;
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid ID. Please enter a numeric ID." << endl;
            return;
        }
        cout << "Enter your password: ";
        cin >> password;
        break;
    case 4:
       
        user.add_Manager();
        break;
    case 5:
        
        user.add_Customer();
        break;
    case 6:
       
        user.add_Employee();
        break;
    case 7:
        {
           cout << "How to use:\n\n"
    << "To run the car rental system, type ./a.out in the terminal.\n\n"
    << "If you want to edit the .cpp file, type g++ as.cpp in the terminal after editing to compile.\n\n"
    << "If you encounter some unexpected behavior or want to exit the car rental system anytime, press Ctr+C.\n\n"
    << "If you see a numbered list on the terminal, enter the number of the functionality you want to use.\n\n"
    << "1. Show the available cars\n"
    << "2. View your rented cars\n"
    << "3. View customer profile\n"
    << "4. Request to rent a car\n"
    << "5. Request to return a car\n"
    << "6. View your fine\n"
    << "7. Logout\n\n"
    << "8. Enter your choice:\n\n"
    << "Follow the instructions you get on the terminal.\n\n"
    << "NOTE:\n"
    << "i) When a customer/employee rents a car, the due date is set to 30 days from the day on which the car was rented(this is the current date. If it is rented on 3rd march then the due date will be 2nd april). If you want to test the fine showing functionality, manually edit the cars.txt file and change the due date. You can change it to feburary(this will ensure that the fine is >0).\n"
    << "ii) The date is stored as yyyy-mm-dd\n"
    << "iii)rating: A+,A implies max 4 cars rentable\n"
    << "    B+,B implies max 3 cars rentable\n"
    << "    C+,C implies max 2 cars rentable\n"
    << "    D+,D implies max 1 cars rentable\n"
    << "iv) fine of 20 rs/day after due date.\n"
    << "v) cars.txt has the data in the format, model,cond,price,rented by(set to Available, if not rented by anyone),duedate(set to None, if not rented by anyone)\n"
    << "vi) managers.txt has the data in the format, name,id,password\n"
    << "vii) employee.txt has the data in the format,name,id,password,rating\n"
    << "viii) customers.txt has the data in the format,name,id,password,rating\n"
    << "ix) class user has class Manager and class Customer_Employee.\n"
    << "x) class Customer_Employee has class Customer and class Employee.\n";


        }
        break;
    case 8:
    {
        std::string continueChoice;
    std::cout << "Do you want to Login again and use car rental system? (1.yes/0.no): ";
    std::cin >> continueChoice;

    if (continueChoice != "1")
    {
        std::cout << "Exiting the program..." << std::endl;
        exit(0); // Exit the program
    }
    else
    {
        // Clear the screen before displaying the role selection menu again
        system("clear");
    }
    break;

    }
    default:
        cout << "Invalid choice." << endl;
        
    }
    

    if (role == "manager")
    {
        Manager manager;
        manager.verifyManagers(id, password);
    }
    else if (role == "customer")
    {
        Customer customer;
        customer.verify_customer(id, password);
    }
    else if (role == "employee")
    {
        Employee employee;
        employee.verify_employee(id, password);
    }
}
