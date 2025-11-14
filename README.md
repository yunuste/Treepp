# 🌳 Tree++

Tree++ is a high-performance, modern file system tree utility re-written in C++17, serving as an equivalent to the original Python **Pytree** tool. This utility allows users to create directories and files based on a hierarchical plain text structure, clean up the created structure, or list and save the existing directory layout in the same format.

## ✨ Features

* **High Performance:** Fast file system operations due to being compiled with C++.
* **Structure Creation:** Creates directories and files according to a simple hierarchical layout defined in a text file.
* **Cleanup:** Safely deletes the entire structure previously created, based on the original template file.
* **Listing & Saving:** Lists the current directory structure or saves it to a file using the Tree++ format.
* **C++17 Standard:** Utilizes modern C++ features, including `std::filesystem`.

## 🛠️ Installation

# 1. Clone the repository
git clone [https://github.com/yunuste/Treepp.git](https://github.com/yunuste/Treepp.git)
cd Treepp

# 2. Build the executable using the Makefile
make

# 3. Install the executable to a system-wide location (/usr/local/bin)
sudo make install
