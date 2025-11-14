# ===============================
# Makefile for Tree++
# ===============================

# Değişkenler
TARGET    = tree++
SOURCE    = tree++.cpp
CXX       = g++
CXXFLAGS  = -std=c++17 -Wall -Wextra -O2
# INSTALL_DIR: Çalıştırılabilir dosyanın kurulacağı yer (Örn: /usr/local/bin)
INSTALL_DIR = /usr/local/bin

# 1. Varsayılan Hedef (make)
all: $(TARGET)

# 2. Çalıştırılabilir Dosyanın Oluşturulması (tree++)
$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET)

# 3. Temizleme (make clean)
clean:
	rm -f $(TARGET)

# 4. Kurulum (make install)
# Bu komut genellikle sudo ile çalıştırılır.
install: $(TARGET)
	# Çalıştırılabilir dosyayı INSTALL_DIR dizinine kopyalar (izinleri 755 olarak ayarlar)
	sudo install -Dm755 $(TARGET) $(INSTALL_DIR)/$(TARGET)

# 5. Kaldırma (make uninstall)
# Bu komut genellikle sudo ile çalıştırılır.
uninstall:
	sudo rm -f $(INSTALL_DIR)/$(TARGET)

# .PHONY, bu hedeflerin gerçek dosyalar olmadığını belirtir.
.PHONY: all clean install uninstall
