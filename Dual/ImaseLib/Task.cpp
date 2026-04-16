//--------------------------------------------------------------------------------------
// File: Task.cpp
//
// タスク
//
// Date: 2026.4.3
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#pragma once
#include "pch.h"
#include "Task.h"
#include "TaskSystem.h"

namespace Imase
{
    // 子追加
    void Task::RequestAddChild(Ptr child)
    {
        if (!m_system) return;

        m_system->RequestAddChild(this, std::move(child));
    }

    // 親変更
    void Task::ChangeParent(Task* newParent)
    {
        if (!newParent) return;
        if (!m_system) return;

        m_system->RequestChangeParent(this, newParent);
    }

    // タスクシステムとIDの設定
    void Task::SetSystem(TaskSystem* sys, uint64_t id)
    {
        m_system = sys;
        m_id = id;
    }

    // 更新
    void Task::UpdateTree(float dt)
    {
        if (!m_active) return;

        // 更新
        if (!Update(dt))
        {
            // タスクを消去
            m_kill = true;
            return;
        }

        // 子の更新
        for (auto& child : m_children)
        {
            child->UpdateTree(dt);
        }
    }

    // 描画対象のリストを作成する関数（親→子）
    void Task::Collect(std::vector<Task*>& out)
    {
        if (m_visible && !m_kill)
        {
            // リストへ登録
            out.push_back(this);
        }

        // 子の登録
        for (auto& child : m_children)
        {
            child->Collect(out);
        }
    }

    // 子のリスト作成
    void Task::CollectAll(Task* t, std::vector<Task*>& out)
    {
        // リストに追加
        out.push_back(t);

        for (auto& child : t->m_children)
        {
            CollectAll(child.get(), out);
        }
    }

    // 削除するタスクリストを作成する関数
    void Task::CollectRemoved(std::vector<Task*>& removed)
    {
        // 削除リストを作成
        for (auto& child : m_children)
        {
            if (child->m_kill)
            {
                CollectAll(child.get(), removed);
            }
            else
            {
                child->CollectRemoved(removed);
            }
        }
    }

    // 死んでいる子を除去する関数
    void Task::Cleanup()
    {
        // 子から
        for (auto& child : m_children)
        {
            child->Cleanup();
        }

        // 子の中から死んでいるタスクを消去
        m_children.erase(std::remove_if(m_children.begin(), m_children.end(),
            [](const Ptr& t) { return t->m_kill; }),
            m_children.end());
    }

    // タグの設定関数
    void Task::SetTag(const std::wstring& tag)
    {
        if (m_system)
        {
            // タグが登録済みならタグを変更する
            m_system->UpdateTag(this, m_tag, tag);
        }
        m_tag = tag;
    }
}
