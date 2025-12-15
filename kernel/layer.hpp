/**
 * @file layer.hpp
 */
#pragma once

#include <memory>
#include <map>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"

// Layerは1つの層を表す
class Layer {
public:
	/**
	 * @brief 指定されたIDを持つレイヤーを生成する
	 */
	Layer(unsigned int id = 0);

	/**
	 * @brief このインスタンスのIDを返す
	 */
	unsigned int ID() const;

	/**
	 * @brief ウィンドウを設定する。既存のウィンドウはこのレイヤから外れる
	 */
	Layer& SetWindow(const std::shared_ptr<Window>& window);

	/**
	 * @brief 設定されたウィンドウを返す
	 */
	std::shared_ptr<Window> GetWindow() const;

	/**
	 * @brief レイヤーの原点座標を取得する
	 */
	Vector2D<int> GetPosition() const;

	/**
	 * @brief trueでレイヤーがドラッグ移動可能となる
	 */
	Layer& SetDraggable(bool draggable);

	/**
	 * @brief レイヤーがドラッグ移動可能ならtrueを返す
	 */
	bool IsDraggable() const;

	/**
	 * @brief レイヤの位置情報を指定した絶対座標へと更新する。再描画はしない
	 */
	Layer& Move(Vector2D<int> pos);

	/**
	 * @brief　レイヤの位置情報を指定した相対座標へと更新する。再描画はしない
	 */
	Layer& MoveRelative(Vector2D<int> pos_diff);

	/**
	 * @brief writerに現在設定されているウィンドウの内容を描画する
	 */
	void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

private:
	unsigned int id_;
	Vector2D<int> pos_{};
	std::shared_ptr<Window> window_{};
	bool draggable_{false};
};

class LayerManager {
public:
	/**
	 * @brief Drawメソッドなどで描画する際の描画先を設定する
	 */
	void SetWriter(FrameBuffer* screen);

	/**
	 * @brief 新しいレイヤを生成して参照を返す
	 */
	Layer& NewLayer();

	/**
	 * @brief 現在表示状態にあるレイヤを描画する
	 */
	void Draw(const Rectangle<int>& area) const;

	/**
	 * @brief 指定したレイヤーに設定されているウィンドウの描画領域内を再描画する
	 */
	void Draw(unsigned int id) const;

	/**
	 * @brief レイヤの位置情報を指定した絶対座標へと更新する。再描画する
	 */
	void Move(unsigned int id, Vector2D<int> new_pos);

	/**
	 * @brief レイヤの位置情報を指定した相対座標へと更新する。再描画する
	 */
	void MoveRelative(unsigned int id, Vector2D<int> pos_diff);

	/**
	 * @brief レイヤの高さ方向の位置を指定した位置に移動する
	 * 
	 * new_heightに負の高さを指定するとレイヤは非表示となり、0以上を指定するとその高さになる
	 * 現在のレイヤ数以上の数値を指定した場合は最前面のレイヤとなる
	 */
	void UpDown(unsigned int id, int new_height);

	/**
	 * @brief レイヤを非表示とする
	 */
	void Hide(unsigned int id);

	Layer* FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;

private:
	FrameBuffer* screen_{nullptr};
	mutable FrameBuffer back_buffer_{};
	// 存在するすべてのレイヤを格納する配列
	std::vector<std::unique_ptr<Layer>> layers_{};
	// 先頭の要素を再背面レイヤ、そこから順に積んでいって、末尾を最前面とするスタック。非表示は含まない
	std::vector<Layer*> layer_stack_{};
	unsigned int latest_id_{0};

	Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;

void InitializeLayer();