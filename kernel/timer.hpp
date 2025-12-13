/**
 * @file timer.hpp
 */
#pragma once

#include <cstdint>

/**
 * @brief Local APICタイマを初期化する
 */
void InitializeLAPICTimer();

/**
 * @brief Local APICタイマ動作を開始する
 */
void StartLAPICTimer();

/**
 * @brief タイマ開始からの経過時間を取得する
 */
uint32_t LAPICTimerElapsed();

/**
 * @brief Local APICタイマの動作を停止する
 */
void StopLAPICTimer();