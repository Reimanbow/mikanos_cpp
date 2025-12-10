/**
 * @file paging.hpp
 */
#pragma once

#include <cstddef>

/**
 * @brief 静的に確保するページディレクトリの個数
 * 
 * この定数はSetupIdentityPageMapで使用する
 * 1つのページディレクトリには512個の2MiBページを設定できるので、
 * kPageDirectoryCount x 1GiBの仮想アドレスがマッピングされることになる
 */
const size_t kPageDirectoryCount = 64;

/**
 * @brief 仮想アドレス=物理アドレスとなるようにページテーブルを設定する
 * 
 * 最終的にCR3レジスタが正しく設定されたページテーブルを指すようになる
 */
void SetupIdentityPageTable();