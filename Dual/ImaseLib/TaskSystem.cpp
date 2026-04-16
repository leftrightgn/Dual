//--------------------------------------------------------------------------------------
// File: TaskSystem.cpp
//
// タスクシステム
//
// Date: 2026.4.3
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#include "pch.h"
#include "TaskSystem.h"
#include "Task.h"
#include <algorithm>

namespace Imase
{
    // コンストラクタ
    TaskSystem::TaskSystem()
    {
        // ルート作成
        m_root = std::make_unique<Task>();
        Register(m_root.get());
    }

    // デストラクタ
    // デストラクタに=defaultを付ける理由
    // ムーブコンストラクタやコピーコンストラクタを明示的に定義した際、
    // デストラクタが暗黙的に生成されなくなる（または動作が変わる）のを防ぐ
    // メンバ変数に前方参照のクラスのunique_ptrがある場合cpp側にデストラクタを記述する
    TaskSystem::~TaskSystem() = default;

    // 子の追加
    void TaskSystem::RequestAddChild(Task* parent, std::unique_ptr<Task> child)
    {
        m_pendingAdd.push_back({ parent, std::move(child) });
    }

    // 親の変更
    void TaskSystem::RequestChangeParent(Task* task, Task* newParent)
    {
        m_pendingChangeParent.push_back({ task, newParent });
    }

    // ID検索
    Task* TaskSystem::FindByID(uint64_t id)
    {
        auto it = m_idMap.find(id);
        return (it != m_idMap.end()) ? it->second : nullptr;
    }

    // タグ検索
    std::vector<Task*> TaskSystem::FindByTag(const std::wstring& tag)
    {
        std::vector<Task*> result;

        auto range = m_tagMap.equal_range(tag);

        result.reserve(std::distance(range.first, range.second));

        for (auto it = range.first; it != range.second; ++it)
        {
            Task* t = it->second;

            if (t && !t->IsKill())
            {
                result.push_back(t);
            }
        }

        return result;
    }

    // タグの変更
    void TaskSystem::UpdateTag(Task* task, const std::wstring& oldTag, const std::wstring& newTag)
    {
        // 古いタグ削除
        if (!oldTag.empty())
        {
            auto range = m_tagMap.equal_range(oldTag);
            for (auto it = range.first; it != range.second; ++it)
            {
                if (it->second == task)
                {
                    m_tagMap.erase(it);
                    break;
                }
            }
        }

        // 新しいタグ登録
        if (!newTag.empty())
        {
            m_tagMap.emplace(newTag, task);
        }
    }

    // タスク登録
    void TaskSystem::Register(Task* task)
    {
        assert(task->GetID() == 0);
        task->SetSystem(this, m_nextID++);
        m_idMap[task->GetID()] = task;

        // タグがある場合だけ登録
        if (!task->GetTag().empty())
        {
            m_tagMap.emplace(task->GetTag(), task);
        }
    }

    // タスク登録解除
    void TaskSystem::Unregister(Task* task)
    {
        // IDMapから削除
        m_idMap.erase(task->GetID());

        // TagMapから削除
        if (!task->GetTag().empty())
        {
            auto range = m_tagMap.equal_range(task->GetTag());

            for (auto it = range.first; it != range.second; )
            {
                if (it->second == task)
                {
                    it = m_tagMap.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    // 更新
    void TaskSystem::Update(float elapsedTime)
    {
        // 更新
        m_root->UpdateTree(elapsedTime);

        //--------------------------------
        // AddChild適用
        //--------------------------------
        for (auto& p : m_pendingAdd)
        {
            // 親が無効 or 死んでいるなら無視
            if (!p.parent || p.parent->IsKill()) continue;

            // 親を設定
            p.child->m_parent = p.parent;

            // ID登録
            Register(p.child.get());

            // 親の子に追加
            p.parent->m_children.emplace_back(std::move(p.child));
        }
        m_pendingAdd.clear();

        //--------------------------------
        // タスクを削除
        //--------------------------------
        std::vector<Task*> removed;

        // 削除するタスクリスト作成
        m_root->CollectRemoved(removed);

        for (auto* task : removed)
        {
            Unregister(task);
            //// IDMapから削除
            //m_idMap.erase(task->GetID());
            //// TagMapから削除
            //if (!task->GetTag().empty())
            //{
            //    auto range = m_tagMap.equal_range(task->GetTag());
            //    for (auto it = range.first; it != range.second;)
            //    {
            //        if (it->second == task)
            //        {
            //            it = m_tagMap.erase(it);
            //        }
            //        else
            //        {
            //            ++it;
            //        }
            //    }
            //}
        }

        // タスクを削除
        m_root->Cleanup();

        //--------------------------------
        // ChangeParent適用
        //--------------------------------
        for (auto& p : m_pendingChangeParent)
        {
            Task* task = p.task;
            Task* newParent = p.newParent;

            // 無効チェック
            if (!task || !newParent || task == newParent) continue;

            // 死亡チェック
            if (task->IsKill() || newParent->IsKill()) continue;

            // ルートは変更不可
            if (!task->m_parent) continue;

            //--------------------------------
            // 循環防止（A（親）←B（子）←A×）
            //--------------------------------
            for (Task* cur = newParent; cur; cur = cur->m_parent)
            {
                if (cur == task)
                {
                    newParent = nullptr;
                    break;
                }
            }
            if (!newParent) continue;

            //--------------------------------
            // 元親から取り外し
            //--------------------------------
            std::vector<Task::Ptr>& siblings = task->m_parent->m_children;

            auto it = std::find_if(siblings.begin(), siblings.end(),
                [task](const Task::Ptr& ptr)
                {
                    return ptr.get() == task;
                });

            if (it == siblings.end()) continue;

            //--------------------------------
            // 所有権移動
            //--------------------------------
            Task::Ptr self = std::move(*it);
            siblings.erase(it);

            //--------------------------------
            // 新しい親へ
            //--------------------------------
            self->m_parent = newParent;
            newParent->m_children.emplace_back(std::move(self));
        }
        m_pendingChangeParent.clear();
    }

    // 描画
    void TaskSystem::Render()
    {
        // 表示＆生きているタスクのリスト作成（親→子）
        std::vector<Task*> list;
        m_root->Collect(list);

        // 描画順をソート
        std::stable_sort(list.begin(), list.end(),
            [](Task* a, Task* b)
            {
                return a->GetOt() < b->GetOt();
            });

        // 描画
        for (auto* task : list)
        {
            task->Render();
        }
    }
}