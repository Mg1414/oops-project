#include "../as.cpp"

#include <cassert>
#include <iostream>

using namespace car_rental;

int main() {
    auto validator = std::make_shared<CarRecordValidator>();
    CarRecordParser parser(validator);

    auto record = parser.parse("car-001,Horizon,excellent,2500,Available,None");
    assert(record.has_value());
    assert(record->id == "car-001");
    assert(record->model == "Horizon");

    try {
        parser.parse("broken,unknown,1000,Available,None");
        assert(false && "invalid condition should throw");
    } catch (const std::exception &) {
    }

    auto backend = std::make_shared<MemoryStorageBackend>();
    CarRepository repository(backend);
    RentalService service(repository);

    service.addCar(*record);
    assert(repository.totalRecords() == 1);

    double quotedAmount = 0;
    bool rentSuccess = service.rentCar(record->id, "user-123", quotedAmount);
    assert(rentSuccess);
    assert(quotedAmount == record->pricePerDay);

    bool returnSuccess = service.returnCar(record->id);
    assert(returnSuccess);

    SyntheticDatasetGenerator generator(validator);
    auto synthetic = generator.generate(25);
    assert(synthetic.size() == 25);
    for (const auto &entry : synthetic) {
        assert(validator->validate(entry));
    }

    std::cout << "unit tests passed" << std::endl;
    return 0;
}
