## Documentation generation
Install the required libraries for generating documentation:
```sh
sudo apt install -y doxygen graphviz
```

Create and activate the `python` virtual environment with `venv`:
```sh
cd docs
python3 -m venv .venv
source .venv/bin/activate
```

Install the `python` packages:
```sh
pip install -r requirements.txt
```

Run the doxygen script to configure the files:
```sh
cd ..
doxygen
```

Then, in the `docs` folder, run:
```sh
cd docs
make clean
python generate_traceability.py
make html
```
