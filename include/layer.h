#ifndef LAYER_H_
#define LAYER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "FreeRTOS.h"
#include "queue.h"

#include "actions.h"
#include "operation.h"

namespace fex
{
    typedef std::unordered_map<
        int,                     // Row * Key
            std::unordered_map<
                Operation,       // Operation
                std::unique_ptr<BoundAction>>>   // Action
        KeyBindings;

    class Layer
    {
    public:
        Layer() = default;

        Layer(Layer &&other) = default;
        Layer &operator=(Layer &&other) = default;

        bool Bound(int key, Operation operation);
        void Bind(int key, std::unique_ptr<BoundAction> action, Operation operation);
        void Enqueue(int key, Operation operation, BoundActionEnqueue action, QueueHandle_t queue);

        const std::string &name() { return name_; }
        const bool on_hold_bound() { return on_hold_bound_; }
        bool unassigned_keys_fall_through() { return unassigned_keys_fall_through_; }
        const KeyBindings& bindings() { return bindings_; }

        void set_name(const std::string &name) { name_ = name; }
        void set_unassigned_keys_fall_through(bool value) { unassigned_keys_fall_through_ = value; }

    private:
        std::string name_;
        bool on_hold_bound_ = false;
        bool unassigned_keys_fall_through_;
        KeyBindings bindings_;
    };

}

#endif