#!/bin/sh

set -e

pip3 install --upgrade -r requirements/testing_requirements.txt

# git lint

echo ""
echo "Running black..."
black --line-length 100 --target-version py37 --diff *.py */*.py
black --line-length 100 --target-version py37 --check *.py */*.py
# black --line-length 100 --target-version py37 *.py */*.py


echo ""
echo "Running flake8..."
flake8 --max-line-length=100 *.py */*.py --ignore E712,W503

#echo ""
#echo "Running mypy..."
#mypy --ignore-missing-imports --strict *.py */*.py

# echo ""
# echo "Running pytest..."
# pip3 install --upgrade -r requirements/requirements.txt
# python3 -m coverage run --source *.py -m pytest -vv -s $@

# python3 -m coverage report -m
