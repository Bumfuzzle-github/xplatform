#pragma once

#include "Coreutils.hpp"

namespace Utils {

    std::string compute_md5(const std::string& message) {
        unsigned char hash[MD5_DIGEST_LENGTH];
        MD5_CTX md5_ctx;
        MD5_Init(&md5_ctx);
        MD5_Update(&md5_ctx, message.c_str(), message.size());
        MD5_Final(hash, &md5_ctx);

        std::stringstream ss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    std::string compute_sha256(const std::string& message) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256_ctx;
        SHA256_Init(&sha256_ctx);
        SHA256_Update(&sha256_ctx, message.c_str(), message.size());
        SHA256_Final(hash, &sha256_ctx);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    std::string merkle_hash(const std::vector<std::string>& hashes) {
        if (hashes.size() == 1) {
            return compute_sha256(hashes[0]);
        }

        std::vector<std::string> temp_hashes = hashes;
        while (temp_hashes.size() > 1) {
            if (temp_hashes.size() % 2 == 1) {
                temp_hashes.push_back(temp_hashes.back());
            }

            std::vector<std::string> new_hashes;
            for (size_t i = 0; i < temp_hashes.size(); i += 2) {
                std::string combined_hash = compute_sha256(temp_hashes[i]) +
                                            compute_sha256(temp_hashes[i + 1]);
                new_hashes.push_back(combined_hash);
            }
            temp_hashes = std::move(new_hashes);
        }
        return temp_hashes[0];
    }

    std::pair<std::string, std::string> mine_block(
        int difficulty,
        int index,
        const std::string& previous_hash,
        const std::vector<std::string>& transactions) {
        
        std::string target(difficulty, '0');
        std::string header = std::to_string(index) +
                             previous_hash +
                             compute_sha256(merkle_hash(transactions));

        auto start_time = std::chrono::high_resolution_clock::now();
        for (int nonce = 0; nonce < INT_MAX; ++nonce) {
            std::string block_hash = compute_sha256(header + std::to_string(nonce));
            std::cout << "Mining..." << block_hash << "\r";

            if (block_hash.substr(0, difficulty) == target) {
                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> elapsed_time = end_time - start_time;

                spdlog::info("Mining successful!");
                std::this_thread::sleep_for(std::chrono::seconds(2));

                std::cout << "Elapsed Time: ~" << elapsed_time.count() << " seconds\n";
                return {block_hash, std::to_string(nonce)};
            }
        }

        spdlog::error("Mining failed");
        return {"", ""};
    }

} 
