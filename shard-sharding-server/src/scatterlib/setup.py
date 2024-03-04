from setuptools import setup, find_packages

setup(
    name="scatter",
    version="0.1",
    description="scatter",
    author="",
    author_email="behrenreich@peratonlabs.com",
    keywords="scatter",
    packages=find_packages(exclude=["contrib", "docs", "tests"]),
    python_requires=">=3.5",
    include_package_data=True,
    scripts=["scatter.py", "optim.py"],
)
