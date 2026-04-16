//--------------------------------------------------------------------------------------
// File: TaskSystem.h
//
// タスクシステム
//
// Date: 2026.4.3
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <cstdint>

namespace Imase
{
    class Task;

    class TaskSystem
    {
    private:

        // ルートタスク
        std::unique_ptr<Task> m_root;

        // 次に追加されるID
        uint64_t m_nextID = 1;

        // 子追加ペンディング
        struct PendingAdd
        {
            Task* parent;
            std::unique_ptr<Task> child;
        };
        std::vector<PendingAdd> m_pendingAdd;

        // 親変更ペンディング
        struct PendingChangeParent
        {
            Task* task;
            Task* newParent;
        };
        std::vector<PendingChangeParent> m_pendingChangeParent;

        // IDとタスクの関連付けテーブル
        std::unordered_map<uint64_t, Task*> m_idMap;

        // Tagとタスクの関連付けテーブル
        std::unordered_multimap<std::wstring, Task*> m_tagMap;

    public:

        // コンストラクタ
        TaskSystem();

        // デストラクタ
        ~TaskSystem();

        // 更新
        void Update(float elapsedTime);

        // 描画
        void Render();

        // 子追加
        void RequestAddChild(Task* parent, std::unique_ptr<Task> child);

        // 親変更
        void RequestChangeParent(Task* task, Task* newParent);

        // ルート取得
        Task* GetRoot() { return m_root.get(); }

        // ID検索
        Task* FindByID(uint64_t id);

        // タグ検索
        std::vector<Task*> FindByTag(const std::wstring& tag);

        // タグの変更
        void UpdateTag(Task* task, const std::wstring& oldTag, const std::wstring& newTag);

    private:

        // ID登録
        void Register(Task* task);

        // ID登録解除
        void Unregister(Task* task);

    };
}
