
## Source code testing
Install the required libraries for testing:
```sh
sudo apt install -y gcc-14 g++-14 make build-essential can-utils clang libssl-dev libcunit1 libcunit1-doc libcunit1-dev
```

Install the `lcov` package:
```sh
curl -LO https://github.com/linux-test-project/lcov/releases/download/v2.3/lcov-2.3.tar.gz
tar -xzf lcov-2.3.tar.gz
cd lcov-2.3
sudo make install
```

Run the script to configure the CAN interface:
```sh
sudo ./setup_vcan.sh
```

Then, in the test folder run:
```sh
make test
```
or
```sh
make coverage
```
