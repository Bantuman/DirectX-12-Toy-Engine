#pragma once

#include <atomic>              // For std::atomic_bool
#include <condition_variable>  // For std::condition_variable.
#include <cstdint>             // For uint64_t

#include <Engine/Core/ThreadSafeQueue.h>

class CommandList;
class Device;

class CommandQueue
{
public:
    // Get an available command list from the command queue.
    std::shared_ptr<CommandList> GetCommandList();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    u64 ExecuteCommandList(std::shared_ptr<CommandList> commandList);
    u64 ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& commandLists);

    u64 Signal();
    bool     IsFenceComplete(u64 fenceValue);
    void     WaitForFenceValue(u64 fenceValue);
    void     Flush();

    // Wait for another command queue to finish.
    void Wait(const CommandQueue& other);

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

    // Only the device can create command queues.
    CommandQueue(Device& device, D3D12_COMMAND_LIST_TYPE type);
    virtual ~CommandQueue();

protected:
    friend struct std::default_delete<CommandQueue>;



private:
    // Free any command lists that are finished processing on the command queue.
    void ProccessInFlightCommandLists();

    // Keep track of command allocators that are "in-flight"
    // The first member is the fence value to wait for, the second is the
    // a shared pointer to the "in-flight" command list.
    using CommandListEntry = std::tuple<u64, std::shared_ptr<CommandList>>;

    Device& m_Device;
    D3D12_COMMAND_LIST_TYPE                    m_CommandListType;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence>        m_d3d12Fence;
    std::atomic_uint64_t                       m_FenceValue;

    ThreadSafeQueue<CommandListEntry>             m_InFlightCommandLists;
    ThreadSafeQueue<std::shared_ptr<CommandList>> m_AvailableCommandLists;

    // A thread to process in-flight command lists.
    std::thread             m_ProcessInFlightCommandListsThread;
    std::atomic_bool        m_bProcessInFlightCommandLists;
    std::mutex              m_ProcessInFlightCommandListsThreadMutex;
    std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};
