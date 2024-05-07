#include <bitset>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <iostream>
#include "../include/core.h"

namespace Core {

    namespace Util {

        bool isLittleEndian(uint8_t byte) {
            return (byte & 1) == 1;
        }

        void save(const std::string& filename, const std::vector<uint8_t>& buffer) {
            std::ofstream outfile(filename, std::ios::binary);
            if (!outfile.is_open()) {
                throw std::ios_base::failure("Failed to open file: " + filename);
            }

            outfile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            outfile.close();
        }

        std::vector<uint8_t> load(const std::string& filepath) {
            std::ifstream infile(filepath, std::ios::binary);
            if (!infile.is_open()) {
                throw std::ios_base::failure("Failed to open file: " + filepath);
            }

            return std::vector<uint8_t>(
                std::istreambuf_iterator<char>(infile),
                std::istreambuf_iterator<char>()
            );
        }

        void retrieveAndSave(ObjectModel::Root* object) {
            if (object == nullptr) 
                throw std::invalid_argument("Cannot retrieve from a null object.");

            int16_t offset = 0;
            std::vector<uint8_t> buffer(object->getSize());
            std::string filename = object->getName() + ".abc";

            object->pack(buffer, offset);
            save(filename, buffer);
        }

    } 

} 
