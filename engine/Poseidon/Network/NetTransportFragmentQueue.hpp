#pragma once

#include <Poseidon/Network/Legacy/netpch.hpp>

namespace Poseidon
{

template <class MessageFactory>
Ref<NetMessage> MergeNetTransportMessageList(NetMessage* msg, MessageFactory&& newMessage)
{
    if (!msg)
    {
        return nullptr;
    }

    unsigned size = 0;
    NetMessage* ptr = msg;
    NetMessage* last = nullptr;
    do
    {
        size += ptr->getLength();
        last = ptr;
    } while ((ptr = ptr->next));

    auto composite = newMessage(size, msg->getChannel());
    if (!composite)
    {
        return nullptr;
    }

    composite->setFrom(last);
    unsigned8* data = (unsigned8*)composite->getData();
    for (ptr = msg; ptr; ptr = ptr->next)
    {
        memcpy(data, ptr->getData(), ptr->getLength());
        data += ptr->getLength();
    }
    composite->setLength(size);
    return composite;
}

template <class MessageRef, class MergeFn>
bool TryAssembleNetTransportMessageFragments(unsigned flags, NetMessage*& msg, MessageRef& split,
                                             NetMessage*& lastSplit, MessageRef& splitUrgent,
                                             NetMessage*& lastSplitUrgent, MergeFn&& mergeMessageListFn)
{
    if ((flags & MSG_PART_FLAG) == 0)
    {
        return true;
    }

    const bool closing = (flags & MSG_CLOSING_FLAG) != 0;
    if ((flags & MSG_URGENT_FLAG) != 0)
    {
        if (closing)
        {
            NET_ERROR(splitUrgent.NotNull());
            lastSplitUrgent->next = msg;
            msg->next = nullptr;
            msg = mergeMessageListFn(splitUrgent);
            splitUrgent = nullptr;
            return true;
        }

        if (!splitUrgent)
        {
            splitUrgent = msg;
        }
        else
        {
            lastSplitUrgent->next = msg;
        }
        (lastSplitUrgent = msg)->next = nullptr;
        return false;
    }

    if (closing)
    {
        NET_ERROR(split.NotNull());
        lastSplit->next = msg;
        msg->next = nullptr;
        msg = mergeMessageListFn(split);
        split = nullptr;
        return true;
    }

    if (!split)
    {
        split = msg;
    }
    else
    {
        lastSplit->next = msg;
    }
    (lastSplit = msg)->next = nullptr;
    return false;
}

} // namespace Poseidon
