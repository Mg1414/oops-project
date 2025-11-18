#!/usr/bin/env python3
"""Dynamic synthetic data generator for the Modular File Processing System.

The generator inspects the current text databases, decides on the next
operation, feeds a single command to the CLI, waits a bit, and repeats.
It keeps a loggedin.txt file so every tiny session looks like:
  login -> (one or more commands) -> logout
"""
from __future__ import annotations

import random
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Set

PROJECT_ROOT = Path(__file__).resolve().parents[1]
CARS_FILE = PROJECT_ROOT / "cars.txt"
CUSTOMERS_FILE = PROJECT_ROOT / "customers.txt"
MANAGERS_FILE = PROJECT_ROOT / "managers.txt"
LOGGEDIN_FILE = PROJECT_ROOT / "loggedin.txt"
CAR_RENTAL_BIN = PROJECT_ROOT / "car_rental"
DEFAULT_INTERVAL = 1  # seconds between commands


def read_lines(path: Path) -> List[str]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8") as handle:
        return [line.strip() for line in handle if line.strip()]


@dataclass
class CarRecord:
    car_id: str
    model: str
    condition: str
    price: float
    status: str

    @property
    def is_available(self) -> bool:
        return self.status == "Available"

    @property
    def renter_id(self) -> Optional[str]:
        stamp = "Rented by the user ID: "
        if self.status.startswith(stamp):
            return self.status[len(stamp):]
        return None


@dataclass
class Customer:
    name: str
    user_id: str
    password: str


@dataclass
class Manager:
    name: str
    user_id: str
    password: str


class FleetState:
    def __init__(self) -> None:
        self.cars: List[CarRecord] = []
        self.customers: List[Customer] = []
        self.managers: List[Manager] = []
        self.cars_by_renter: Dict[str, List[CarRecord]] = {}
        self._load()

    def _load(self) -> None:
        self._load_cars()
        self._load_customers()
        self._load_managers()

    def reload(self) -> None:
        self._load()

    def _load_cars(self) -> None:
        self.cars.clear()
        self.cars_by_renter.clear()
        for raw in read_lines(CARS_FILE):
            parts = [segment.strip() for segment in raw.split(",")]
            if len(parts) < 4:
                continue
            car_id = parts[0]
            if len(parts) == 4:
                model = car_id
                condition = parts[1]
                price_str = parts[2]
                status = parts[3]
            else:
                model = parts[1]
                condition = parts[2]
                price_str = parts[3]
                status = parts[4]
            try:
                price = float(price_str)
            except ValueError:
                price = 0.0
            record = CarRecord(car_id=car_id, model=model, condition=condition,
                               price=price, status=status)
            self.cars.append(record)
            renter = record.renter_id
            if renter:
                self.cars_by_renter.setdefault(renter, []).append(record)

    def _load_customers(self) -> None:
        self.customers = []
        for raw in read_lines(CUSTOMERS_FILE):
            parts = raw.split(",")
            if len(parts) < 3:
                continue
            name = parts[0].strip()
            user_id = parts[1].strip()
            password = parts[2].strip()
            self.customers.append(Customer(name, user_id, password))

    def _load_managers(self) -> None:
        self.managers = []
        for raw in read_lines(MANAGERS_FILE):
            parts = raw.split(",")
            if len(parts) < 3:
                continue
            name = parts[0].strip()
            user_id = parts[1].strip()
            password = parts[2].strip()
            self.managers.append(Manager(name, user_id, password))

    def available_cars(self) -> List[CarRecord]:
        return [car for car in self.cars if car.is_available]

    def cars_rented_by(self, user_id: str) -> List[CarRecord]:
        return list(self.cars_by_renter.get(user_id, []))

    def next_car_id(self) -> str:
        existing_ids = {car.car_id for car in self.cars}
        index = len(existing_ids) + 1
        while True:
            candidate = f"auto-{index}"
            if candidate not in existing_ids:
                return candidate
            index += 1

    def next_customer_id(self) -> str:
        existing = {cust.user_id for cust in self.customers}
        index = len(existing) + 1000
        while True:
            candidate = f"C{index}"
            if candidate not in existing:
                return candidate
            index += 1

    def write_cars(self) -> None:
        with CARS_FILE.open("w", encoding="utf-8") as handle:
            for car in self.cars:
                handle.write(
                    f"{car.car_id},{car.model},{car.condition},{car.price:.0f},"
                    f"{car.status}\n"
                )

    def write_customers(self) -> None:
        with CUSTOMERS_FILE.open("w", encoding="utf-8") as handle:
            for cust in self.customers:
                handle.write(
                    f"{cust.name},{cust.user_id},{cust.password}\n"
                )


def load_loggedin() -> Dict[str, Set[str]]:
    logged: Dict[str, Set[str]] = {"customer": set(), "manager": set()}
    for raw in read_lines(LOGGEDIN_FILE):
        try:
            role, user_id = raw.split(":", 1)
            logged.setdefault(role, set()).add(user_id)
        except ValueError:
            continue
    return logged


def save_loggedin(logged: Dict[str, Set[str]]) -> None:
    lines: List[str] = []
    for role, ids in logged.items():
        for user_id in sorted(ids):
            lines.append(f"{role}:{user_id}")
    with LOGGEDIN_FILE.open("w", encoding="utf-8") as handle:
        handle.write("\n".join(lines))


def run_cli_command(command: str) -> None:
    payload = f"{command}\nsave\nexit\n"
    subprocess.run([str(CAR_RENTAL_BIN), "--backend=file"],
                   input=payload,
                   text=True,
                   cwd=PROJECT_ROOT,
                   check=False)


def login_role(role: str, user_id: str) -> None:
    logged = load_loggedin()
    if user_id in logged.get(role, set()):
        return
    logged.setdefault(role, set()).add(user_id)
    save_loggedin(logged)
    print(f"[LOGIN] {role} {user_id}")


def logout_role(role: str, user_id: str) -> None:
    logged = load_loggedin()
    if user_id in logged.get(role, set()):
        logged[role].remove(user_id)
        save_loggedin(logged)
        print(f"[LOGOUT] {role} {user_id}")


def choose_logged_in_customer(state: FleetState, logged: Dict[str, Set[str]]) -> Optional[Customer]:
    logged_ids = list(logged.get("customer", set()))
    if not logged_ids:
        return None
    id_to_customer = {cust.user_id: cust for cust in state.customers}
    random.shuffle(logged_ids)
    for cid in logged_ids:
        customer = id_to_customer.get(cid)
        if customer:
            return customer
    return None


def choose_customer_to_login(state: FleetState, logged: Dict[str, Set[str]]) -> Optional[Customer]:
    available = [c for c in state.customers if c.user_id not in logged.get("customer", set())]
    if not available:
        return None
    return random.choice(available)


def choose_manager(state: FleetState, logged: Dict[str, Set[str]]) -> Optional[Manager]:
    if not state.managers:
        return None
    available = [m for m in state.managers if m.user_id not in logged.get("manager", set())]
    if not available:
        return None
    return random.choice(available)


def customer_command(state: FleetState, customer: Customer) -> Optional[str]:
    rentals = state.cars_rented_by(customer.user_id)
    available = state.available_cars()
    dice = random.random()
    if rentals and dice < 0.4:
        car = random.choice(rentals)
        return f"return {car.car_id}"
    if available and dice < 0.9:
        car = random.choice(available)
        return f"rent {car.car_id} {customer.user_id}"
    return "list 5"


def manager_action(state: FleetState) -> str:
    car_id = state.next_car_id()
    model = random.choice(["Atlas", "Falcon", "Nimbus", "Comet", "Vertex"])
    condition = random.choice(["excellent", "good", "fair", "minordamages"])
    price = random.randint(2500, 7800)
    return f"add {car_id} {model} {condition} {price}"


def add_customer_action(state: FleetState) -> None:
    state.reload()
    user_id = state.next_customer_id()
    name = f"AutoCustomer{user_id[-3:]}"
    password = f"auto{user_id[-3:]}"
    state.customers.append(Customer(name, user_id, password))
    state.write_customers()
    print(f"[ADD-CUSTOMER] {user_id}")
    # ensure the new customer starts with a car if possible
    state.reload()
    available = state.available_cars()
    if available:
        car = random.choice(available)
        cmd = f"rent {car.car_id} {user_id}"
        print(f"[CMD] {cmd}")
        run_cli_command(cmd)


def add_car_action(state: FleetState) -> None:
    state.reload()
    cmd = manager_action(state)
    print(f"[CMD] {cmd}")
    run_cli_command(cmd)


def remove_available_car_action(state: FleetState) -> None:
    state.reload()
    available = state.available_cars()
    if not available:
        print("[INFO] No available car to remove.")
        return
    car = random.choice(available)
    state.cars = [c for c in state.cars if c.car_id != car.car_id]
    state.write_cars()
    print(f"[REMOVE-CAR] {car.car_id}")


def force_remove_customer_action(state: FleetState) -> None:
    state.reload()
    if not state.customers:
        print("[INFO] No customer to remove.")
        return
    victim = random.choice(state.customers)
    state.customers = [c for c in state.customers if c.user_id != victim.user_id]
    # release their cars
    for car in state.cars:
        if car.renter_id == victim.user_id:
            car.status = "Available"
    state.write_customers()
    state.write_cars()
    logout_role("customer", victim.user_id)
    print(f"[REMOVE-CUSTOMER] {victim.user_id}")


def login_customer_action(state: FleetState) -> None:
    state.reload()
    logged = load_loggedin()
    candidate = choose_customer_to_login(state, logged)
    if not candidate:
        print("[INFO] No offline customer to login.")
        return
    login_role("customer", candidate.user_id)


def customer_session_action(interval: int) -> None:
    state = FleetState()
    logged = load_loggedin()
    customer = choose_logged_in_customer(state, logged)
    if not customer:
        print("[INFO] No logged-in customer available.")
        return
    login_role("customer", customer.user_id)
    state.reload()
    command = customer_command(state, customer)
    if not command:
        return
    print(f"[CMD] {command}")
    run_cli_command(command)


def main() -> None:
    interval = DEFAULT_INTERVAL
    random.seed()
    while True:
        try:
            roll = random.random()
            if roll < 0.45:
                customer_session_action(interval)
            elif roll < 0.55:
                login_customer_action(FleetState())
            elif roll < 0.65:
                add_customer_action(FleetState())
            elif roll < 0.75:
                add_car_action(FleetState())
            elif roll < 0.85:
                remove_available_car_action(FleetState())
            else:
                force_remove_customer_action(FleetState())
        except Exception as exc:  # pragma: no cover - best-effort daemon
            print(f"[ERROR] {exc}")
        finally:
            time.sleep(interval)


if __name__ == "__main__":
    main()
