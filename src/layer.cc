#include "layer.h"

namespace fex
{
    bool Layer::Bound(int key, Operation operation)
    {
        auto row_key_it = bindings_.find(key);
        if (row_key_it == bindings_.end()) return false;

        return row_key_it->second.find(operation) != row_key_it->second.end();
    }

    void Layer::Bind(int key, std::unique_ptr<BoundAction> action, Operation operation)
    {
        if (operation == Operation::HOLD) {
            on_hold_bound_ = true;
        }
        auto row_key_it = bindings_.find(key);
        if (row_key_it == bindings_.end())
        {
            std::unordered_map<Operation, std::unique_ptr<BoundAction>> op_action;
            op_action.insert({operation, std::move(action)});

            bindings_.insert({key, std::move(op_action)});

            return;
        }

        row_key_it->second.insert({operation, std::move(action)});
    }

    void Layer::Enqueue(int key, Operation operation, BoundActionEnqueue action, QueueHandle_t queue)
    {
        printf("Firing action: %d op: %d\n", key, operation);
        auto it = bindings_.find(key);
        if (it == bindings_.end())
        {
            printf("unbound key\n");
            return;
        }

        auto op_it = it->second.find(operation);
        if (op_it == it->second.end())
        {
            printf("unbound operation\n");
            return;
        }

        op_it->second->Print();
        op_it->second->Enqueue(action, queue);
    }

}