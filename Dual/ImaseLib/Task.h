//--------------------------------------------------------------------------------------
// File: Task.h
//
// タスク
//
// Date: 2026.4.3
// Author: Hideyasu Imase
//--------------------------------------------------------------------------------------
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace Imase
{
    class TaskSystem;

    class Task
    {
    public:
        using Ptr = std::unique_ptr<Task>;

    private:
        
        // タスクシステムへのポインタ
        TaskSystem* m_system = nullptr;
        
        // 親タスクへのポインタ
        Task* m_parent = nullptr;
        
        // 子タスクへのポインタの配列
        std::vector<Ptr> m_children;

        // ID
        uint64_t m_id = 0;

        // タグ
        std::wstring m_tag;
        
        // タスクの名前
        std::wstring m_name;
        
        // Updateを実行する場合はtrue
        bool m_active = true;

        // Renderを実行する場合はtrue
        bool m_visible = true;

        // 死んでいる場合はtrue
        bool m_kill = false;

        // 描画順
        int m_ot = 0;
  
    public:

        // デストラクタ
        virtual ~Task() = default;

        //--------------------------------
        // オーバーライドして使用する関数
        //--------------------------------
        // 更新（戻り値がfalseの場合タスクが消える）
        virtual bool Update(float) { return true; }
        // 描画
        virtual void Render() {}

        //--------------------------------
        // 基本
        //--------------------------------

        // ID取得
        uint64_t GetID() const { return m_id; }

        // タスクを殺す関数
        void Kill() { m_kill = true; }
        // タスクが死んでいるか調べる関数
        bool IsKill() const { return m_kill; }
        
        // 描画順を設定
        void SetOt(int v) { m_ot = v; }
        // 描画順を取得
        int GetOt() const { return m_ot; }

        // タグを設定
        void SetTag(const std::wstring& tag);
        // タグを取得
        const std::wstring& GetTag() const { return m_tag; }
        
        // タスクに名前を付ける関数
        void SetName(const std::wstring& name) { m_name = name; }
        // タスクの名前を取得する関数
        const std::wstring& GetName() { return m_name; }

        // 子追加
        template<class T, class... Args>
        T* AddChild(Args&&... args)
        {
            auto child = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = child.get();

            RequestAddChild(std::move(child));
            return raw;
        }

        // 親変更
        void ChangeParent(Task* newParent);
   
        // 親タスクを取得する関数
        Task* GetParent() { return m_parent; }

    private:
        //--------------------------------
        // タスクシステム内部用
        //--------------------------------
        void RequestAddChild(Ptr child);

        // タスクシステムとIDの設定
        void SetSystem(TaskSystem* sys, uint64_t id);

        // 更新
        void UpdateTree(float dt);

        // 描画対象のリストを作成する関数（親→子）
        void Collect(std::vector<Task*>& out);

        // 子のリスト作成
        void CollectAll(Task* t, std::vector<Task*>& out);

        // 削除するタスクリストを作成する関数
        void CollectRemoved(std::vector<Task*>& removed);

        // 死んでいる子を除去する関数
        void Cleanup();

        friend class TaskSystem;
    };
}
