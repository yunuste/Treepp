#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>

// C++17 Filesystem Namespace
namespace fs = std::filesystem;

// --- GLOBAL CONSTANTS ---
const std::vector<std::string> TREE_SYMBOLS_AND_SPACES = {"├──", "└──", "│", " ", " "};

// Ağaç yapısındaki bir öğeyi temsil eden struct
struct TreeItem {
    std::string name;
    fs::path path;
    bool is_dir;
    int level;
};

// --- HELPER FUNCTIONS ---

/**
 * Açıklamadan sonraki satırın kısmını kaldırır.
 */
std::string clean_line_of_comments(const std::string& line) {
    size_t hash_pos = line.find('#');
    size_t slash_pos = line.find("//");

    // İlgili yorum pozisyonlarını bul
    size_t first_comment_pos = std::string::npos;
    if (hash_pos != std::string::npos) {
        first_comment_pos = hash_pos;
    }
    if (slash_pos != std::string::npos) {
        if (first_comment_pos == std::string::npos || slash_pos < first_comment_pos) {
            first_comment_pos = slash_pos;
        }
    }

    std::string result = (first_comment_pos != std::string::npos)
                         ? line.substr(0, first_comment_pos)
                         : line;

    // Sondaki boşlukları kaldırma
    size_t end = result.find_last_not_of(" \t\n\r\f\v");
    if (end == std::string::npos) return ""; // Sadece boşluksa
    return result.substr(0, end + 1);
}

/**
 * Tree++ dosyasından öğeleri okur ve TreeItem vektörünü döndürür.
 */
std::vector<TreeItem> get_tree_items_from_file(const fs::path& pytree_file) {
    std::vector<TreeItem> items;
    std::ifstream file(pytree_file);

    if (!file.is_open()) {
        std::cerr << "Error: Structure file not found -> " << pytree_file.string() << std::endl;
        return items;
    }

    std::string raw_line;
    std::vector<std::string> path_stack;
    std::map<int, int> level_index_map; // <level, item_start_index>

    while (std::getline(file, raw_line)) {
        std::string line_no_comments = clean_line_of_comments(raw_line);

        if (line_no_comments.empty()) continue;

        // 1. Öğenin başlangıç indeksini bulma
        int item_start_index = -1;
        for (int i = 0; i < line_no_comments.length(); ++i) {
            char c = line_no_comments[i];
            // Python'daki char.isalnum() or char in ['.', '-', '_', '/', '{', '}'] karşılığı
            if (std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '/' || c == '{' || c == '}') {
                item_start_index = i;
                break;
            }
        }

        if (item_start_index == -1) continue;

        // 2. Saf öğe adını belirleme
        std::string item_name_raw = line_no_comments.substr(item_start_index);
        std::string item_name = item_name_raw;

        for (const auto& symbol : TREE_SYMBOLS_AND_SPACES) {
            size_t pos = item_name.find(symbol);
            while (pos != std::string::npos) {
                item_name.replace(pos, symbol.length(), "");
                pos = item_name.find(symbol);
            }
        }
        
        // Sondaki boşlukları kaldırma
        size_t name_end = item_name.find_last_not_of(" \t\n\r\f\v");
        item_name = (name_end == std::string::npos) ? "" : item_name.substr(0, name_end + 1);

        if (item_name.empty()) continue;

        // 3. Seviye (Level) Belirleme
        int current_level = -1;
        if (level_index_map.empty()) {
            current_level = 0;
            level_index_map[0] = item_start_index;
        } else {
            bool found_level = false;
            for (const auto& pair : level_index_map) {
                if (std::abs(item_start_index - pair.second) < 2) { // Yakın indeks tespiti
                    current_level = pair.first;
                    found_level = true;
                    break;
                }
            }
            if (!found_level) {
                int new_level = level_index_map.rbegin()->first + 1;
                level_index_map[new_level] = item_start_index;
                current_level = new_level;
            }
        }

        // 4. Stack Yönetimi
        while (path_stack.size() > current_level) {
            path_stack.pop_back();
        }

        // 5. Tam Yolu Oluşturma
        bool is_dir = false;
        if (!item_name_raw.empty()) {
            // C++17 uyumlu ends_with kontrolü
            is_dir = (item_name_raw.back() == '/'); 
        }
        
        // item_name'in sonundaki '/' karakterini kaldırma
        std::string clean_name = item_name;
        // is_dir kontrolünü de ekledim ki sadece dizinlerde '/' kaldırılsın.
        if (is_dir && !clean_name.empty() && clean_name.back() == '/') {
            clean_name.pop_back();
        }

        fs::path full_path = fs::current_path();
        for (const auto& dir : path_stack) {
            full_path /= dir;
        }
        full_path /= clean_name;

        // 6. Sonraki adım için Stack'i Güncelleme
        if (is_dir) {
            path_stack.push_back(clean_name);
        }

        items.push_back({
            clean_name,
            full_path,
            is_dir,
            current_level
        });
    }

    return items;
}

// --- 1. Creation Function (-c) ---
void create_tree_from_file(const fs::path& pytree_file, bool clean_file) {
    std::cout << "Creating file structure based on '" << pytree_file.string() << "'..." << std::endl;
    std::vector<TreeItem> items = get_tree_items_from_file(pytree_file);

    if (items.empty()) {
        if (fs::exists(pytree_file)) return;
        // Hata mesajı get_tree_items_from_file içinde basıldı.
        return;
    }

    for (const auto& item : items) {
        try {
            if (item.is_dir) {
                // Python'daki os.makedirs(..., exist_ok=True) karşılığı
                fs::create_directories(item.path);
                std::cout << "  [Directory Created] " << item.path.string() << std::endl;
            } else {
                // Python'daki path.touch(exist_ok=True) karşılığı
                std::ofstream ofs(item.path);
                if (!ofs.is_open()) throw std::runtime_error("File creation failed.");
                ofs.close();
                std::cout << "  [File Created] " << item.path.string() << std::endl;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "  [ERROR] Could not create '" << item.name << "': " << e.what() << std::endl;
        } catch (const std::exception& e) {
             std::cerr << "  [ERROR] Could not create '" << item.name << "': " << e.what() << std::endl;
        }
    }

    std::cout << "\nStructure creation completed." << std::endl;

    if (clean_file) {
        try {
            // Python'daki os.remove(pytree_file) karşılığı
            fs::remove(pytree_file);
            std::cout << "Source file successfully deleted: " << pytree_file.string() << std::endl;
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: Could not delete source file: " << e.what() << std::endl;
        }
    }
}

// --- 2. Deletion/Cleanup Function (-clean) ---
void clean_created_tree(const fs::path& pytree_file) {
    std::cout << "Deleting structure created according to '" << pytree_file.string() << "'..." << std::endl;
    std::vector<TreeItem> items = get_tree_items_from_file(pytree_file);

    if (items.empty()) return;

    // Ters sıralama (içteki öğeleri önce silmek için)
    std::sort(items.begin(), items.end(), [](const TreeItem& a, const TreeItem& b) {
        return a.level > b.level;
    });

    std::set<fs::path> deleted_dirs; // Silinmiş dizinleri takip et

    for (const auto& item : items) {
        try {
            // Halihazırda silinmiş bir dizinin çocuğu olup olmadığını kontrol et
            bool is_child_of_deleted = false;
            for (const auto& d : deleted_dirs) {
                // Python'daki path.is_relative_to(d) karşılığı (Basit string önek kontrolü)
                std::string item_str = item.path.string();
                std::string deleted_str = d.string();
                if (item_str.size() >= deleted_str.size() && item_str.rfind(deleted_str, 0) == 0) { 
                    is_child_of_deleted = true;
                    break;
                }
            }
            if (is_child_of_deleted) continue;

            if (item.is_dir) {
                // Python'daki shutil.rmtree(path) karşılığı
                if (fs::exists(item.path)) {
                    fs::remove_all(item.path);
                    deleted_dirs.insert(item.path);
                    std::cout << "  [Directory Deleted] " << item.path.string() << std::endl;
                }
            } else {
                // Python'daki os.remove(path) karşılığı
                if (fs::exists(item.path)) {
                    fs::remove(item.path);
                    std::cout << "  [File Deleted] " << item.path.string() << std::endl;
                }
            }

        } catch (const fs::filesystem_error& e) {
            // Dosya bulunamadı hatası atla
            if (e.code() == std::make_error_code(std::errc::no_such_file_or_directory)) {
                continue;
            }
            std::cerr << "  [ERROR] Could not delete '" << item.name << "': " << e.what() << std::endl;
        }
    }

    std::cout << "\nCleanup completed." << std::endl;
}

// --- 3. Listing Function (-l and -s) ---
void generate_lines(const fs::path& current_dir, const std::string& current_prefix, std::vector<std::string>& output_lines) {
    try {
        std::vector<fs::directory_entry> contents;
        for (const auto& entry : fs::directory_iterator(current_dir)) {
            // Gizli dosya ve klasörleri atla (Unix/Linux'taki .)
            if (entry.path().filename().string().rfind('.', 0) != 0) {
                contents.push_back(entry);
            }
        }
        
        // İsimlere göre sıralama
        std::sort(contents.begin(), contents.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            return a.path().filename().string() < b.path().filename().string();
        });

        for (size_t i = 0; i < contents.size(); ++i) {
            const auto& entry = contents[i];
            bool is_last = (i == contents.size() - 1);

            std::string pointer = is_last ? "└── " : "├── ";
            std::string item_name = entry.path().filename().string();
            std::string line = current_prefix + pointer + item_name;
            output_lines.push_back(line);

            if (entry.is_directory()) {
                std::string new_prefix = current_prefix + (is_last ? "    " : "│   ");
                generate_lines(entry.path(), new_prefix, output_lines);
            }
        }
    } catch (const fs::filesystem_error& e) {
        output_lines.push_back("Error: Directory not found or Permission denied -> " + current_dir.string());
    }
}

void list_tree(const fs::path& start_dir, const fs::path& output_file = "") {
    std::vector<std::string> output_lines;
    generate_lines(start_dir, "", output_lines);

    // Çıktıyı yazdırma veya dosyaya kaydetme
    if (!output_file.empty()) {
        try {
            std::ofstream ofs(output_file);
            if (!ofs.is_open()) throw std::runtime_error("File creation failed.");
            
            for (const auto& line : output_lines) {
                ofs << line << "\n";
            }
            std::cout << "Tree structure successfully saved: " << output_file.string() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: Failed to write to file: " << e.what() << std::endl;
        }
    } else {
        // Standart Liste Modu
        std::cout << "Tree structure of: " << start_dir.string() << std::endl;
        for (const auto& line : output_lines) {
            std::cout << line << std::endl;
        }
    }
}

// --- 4. Main Program (Argument Handling) ---
void show_help(const std::string& program_name) {
    std::cout << "C++ File System Tree (Tree++) Tool." << std::endl;
    std::cout << "Usage: " << program_name << " [OPTION] [FILE|DIRECTORY]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -c <FILE>         Creates the tree structure from the specified file." << std::endl;
    std::cout << "  -clean [FILE]     If used with -c, deletes the source file after creation." << std::endl;
    std::cout << "                    If used alone (e.g., -clean main.tree), deletes the structure previously created." << std::endl;
    std::cout << "  -l                Lists the current directory structure to standard output." << std::endl;
    std::cout << "  -s <OUTPUT_FILE>  Saves the current directory structure to the specified file." << std::endl;
    std::cout << "  <DIRECTORY>       The directory path to operate on (default: .)." << std::endl;
    std::cout << "\nExample Usage:\n";
    std::cout << "  ./tree++ -c main.tree (Creates the structure)\n";
    std::cout << "  ./tree++ -c main.tree -clean (Creates structure and deletes main.tree)\n";
    std::cout << "  ./tree++ -clean main.tree (Deletes the created structure based on main.tree)\n";
    std::cout << "  ./tree++ -l (Lists the current directory)\n";
}

int main(int argc, char* argv[]) {
    // Argüman sayısının 1 olması (sadece program adı) veya help istenmesi durumunda
    if (argc == 1 || (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help"))) {
        show_help(argv[0]);
        return 0;
    }

    // Basit komut satırı argümanı ayrıştırma
    std::string command = argv[1];
    std::string target_file_or_dir;
    bool clean_flag = false;
    
    // Argümanları al
    if (argc > 2) {
        // -c <FILE> veya -clean <FILE> veya -s <FILE> veya -l <DIR> için
        if (command == "-c" || command == "-clean" || command == "-s" || command == "-l") {
            if (argc > 2) target_file_or_dir = argv[2];
        } else {
            // Varsayılan dizin argümanı (pytree .)
            target_file_or_dir = command;
        }
    }

    // Daha fazla argüman varsa, temizleme bayrağını kontrol et
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-clean") {
            clean_flag = true;
            // Eğer -clean tek başına ve dosya ile kullanıldıysa, hedefi doğru ayarla
            if (command == "-clean" && argc > 2) {
                target_file_or_dir = argv[2];
            } else if (command != "-c" && argc > 2 && target_file_or_dir.empty()) {
                // Sadece -clean komutunun hemen ardından dosya geliyorsa (./tree++ -clean main.tree)
                target_file_or_dir = argv[2];
            }
            break;
        }
    }
    
    // Command'i tekrar kontrol et, çünkü target_file_or_dir karışmış olabilir
    command = argv[1];

    // 1. Cleanup Modu (./tree++ -clean main.tree)
    if (command == "-clean" && !target_file_or_dir.empty()) { // Tek başına temizleme modu
        clean_created_tree(target_file_or_dir);
    }
    // 2. Creation Modu (./tree++ -c main.tree)
    else if (command == "-c" && !target_file_or_dir.empty()) {
        create_tree_from_file(target_file_or_dir, clean_flag);
    }
    // 3. Listing/Saving Modu
    else if (command == "-s") {
        fs::path output_file = target_file_or_dir;
        if (output_file.empty()) {
             std::cerr << "Error: -s option requires an output file name." << std::endl;
             show_help(argv[0]);
             return 1;
        }
        if (output_file.extension().string() != ".pytree") {
            output_file += ".pytree";
        }
        list_tree(fs::current_path(), output_file);
    }
    else if (command == "-l") {
        fs::path dir = (target_file_or_dir.empty() || target_file_or_dir == "-l") ? "." : target_file_or_dir;
        list_tree(dir);
    }
    else {
        // Tüm senaryoları kapsamazsa yardım göster
        show_help(argv[0]);
    }

    return 0;
}
