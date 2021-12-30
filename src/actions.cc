#include "actions.h"

#include <memory>
#include <string>
#include <vector>

#include "FreeRTOS.h"
#include "queue.h"

#include "queue_message.h"

namespace fex
{

        void GenericKeyAction::Print() const
        {
                printf("GenericKeyAction\n");
                for (int code : keycodes_)
                {
                        printf("\t - %02X\n", code);
                }
        }

        void GenericKeyAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                QueueMessage msg;

                switch (action)
                {
                case BoundActionEnqueue::DO:
                        printf("DO action\n");
                        msg.type = MessageType::PRESS;
                        break;

                case BoundActionEnqueue::UNDO:
                        printf("UNDO action\n");
                        msg.type = MessageType::RELEASE;
                        break;

                default:
                        break;
                }

                for (int i = 0; i < KEY_ROLL_OVER; i++)
                {
                        // TODO(fex): Keys over KEY_ROLL_OVER will be lost
                        msg.codes[i] = keycodes_[i];
                }

                msg.length = (keycodes_.size() > KEY_ROLL_OVER) ? KEY_ROLL_OVER : keycodes_.size();

                xQueueSend(queue, (void *)&msg, 10);
        }

        bool GenericKeyAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::GENERIC_KEY_ACTION)
                {
                        return false;
                }

                const GenericKeyAction *o = static_cast<const GenericKeyAction *>(&other);
                return keycodes() == o->keycodes();
        }

        void PressKeyAction::Print() const
        {
                printf("PressKeyAction\n");
        }

        void PressKeyAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                // Only send the 'key down' even for press
                if (action == BoundActionEnqueue::DO)
                {
                        printf("PRESS action\n");
                        GenericKeyAction::Enqueue(action, queue);
                }
        }

        bool PressKeyAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::PRESS_KEY_ACTION)
                {
                        return false;
                }

                const PressKeyAction *o = static_cast<const PressKeyAction *>(&other);
                return keycodes() == o->keycodes();
        }

        void ReleaseKeyAction::Print() const
        {
                printf("ReleaseKeyAction\n");
        }

        void ReleaseKeyAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                // Only send the 'key up' even for release
                if (action == BoundActionEnqueue::DO)
                {
                        printf("RELEASE action\n");
                        GenericKeyAction::Enqueue(BoundActionEnqueue::UNDO, queue);
                }
        }

        bool ReleaseKeyAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::RELEASE_KEY_ACTION)
                {
                        return false;
                }

                const ReleaseKeyAction *o = static_cast<const ReleaseKeyAction *>(&other);
                return keycodes() == o->keycodes();
        }

        void ClickKeyAction::Print() const
        {
                printf("ClickKeyAction\n");
        }

        void ClickKeyAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        printf("CLICK action\n");
                        GenericKeyAction::Enqueue(action, queue);
                        GenericKeyAction::Enqueue(BoundActionEnqueue::UNDO, queue);
                }
        }

        bool ClickKeyAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::CLICK_KEY_ACTION)
                {
                        return false;
                }

                const ClickKeyAction *o = static_cast<const ClickKeyAction *>(&other);
                return keycodes() == o->keycodes();
        }

        void SequenceAction::Print() const
        {
                printf("SequenceAction(%d):\n", type_);
                for (const auto &i : sequence_)
                {
                        printf("\t");
                        i->Print();
                }
        }

        void SequenceAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                // Send DO and UNDO back to back, keys in a sequence should not be press
                // at the same time
                if (action == BoundActionEnqueue::DO)
                {
                        printf("DO Sequence action\n");
                        for (const auto &item : sequence_)
                        {
                                item->Enqueue(BoundActionEnqueue::DO, queue);
                                item->Enqueue(BoundActionEnqueue::UNDO, queue);
                        }
                }
        }

        bool SequenceAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::SEQUENCE_ACTION)
                {
                        return false;
                }

                const SequenceAction *o = static_cast<const SequenceAction *>(&other);

                if (sequence().size() != o->sequence().size())
                {
                        return false;
                }

                for (int i = 0; i < sequence().size(); i++)
                {
                        if (!(*sequence()[i] == *o->sequence()[i]))
                        {
                                return false;
                        }
                }

                return true;
        }

        void DelayAction::Print() const
        {
                printf("DelayAction\n");
        }

        void DelayAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        printf("DO delay action\n");
                        QueueMessage msg;
                        msg.type = MessageType::DELAY;
                        msg.delay = duration_;
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool DelayAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::DELAY_ACTION)
                {
                        return false;
                }

                const DelayAction *o = static_cast<const DelayAction *>(&other);
                return duration() == o->duration();
        }

        void GenericLayerAction::Print() const
        {
                printf("GenericLayerAction\n");
        }

        void GenericLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool GenericLayerAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::GENERIC_LAYER_ACTION)
                {
                        return false;
                }

                const GenericLayerAction *o = static_cast<const GenericLayerAction *>(&other);
                return target_layer() == o->target_layer();
        }

        void SwitchToLayerAction::Print() const
        {
                printf("SwitchToLayerAction: %s\n", target_layer_.c_str());
        }

        void SwitchToLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        printf("Switch to: %s\n", target_layer_.c_str());
                        QueueMessage msg;

                        msg.type = MessageType::LAYER_SWITCH;
                        msg.layer = std::hash<std::string>()(target_layer_);
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool SwitchToLayerAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::SWITCH_TO_LAYER_ACTION)
                {
                        return false;
                }

                const SwitchToLayerAction *o = static_cast<const SwitchToLayerAction *>(&other);
                return target_layer() == o->target_layer();
        }

        void TemporaryLayerAction::Print() const
        {
                printf("TemporaryLayerAction(%d): %s\n", type_, target_layer_.c_str());
        }

        void TemporaryLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool TemporaryLayerAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::TEMPORARY_LAYER_ACTION)
                {
                        return false;
                }

                const TemporaryLayerAction *o = static_cast<const TemporaryLayerAction *>(&other);
                return target_layer() == o->target_layer();
        }

        void LeaveLayerAction::Print() const
        {
                printf("LeaveLayerAction(%d): %s\n", type_, target_layer_.c_str());
        }

        void LeaveLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool LeaveLayerAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::LEAVE_LAYER_ACTION)
                {
                        return false;
                }

                const LeaveLayerAction *o = static_cast<const LeaveLayerAction *>(&other);
                return target_layer() == o->target_layer();
        }

        void ToggleLayerAction::Print() const
        {
                printf("ToggleLayerAction(%d): %s\n", type_, target_layer_.c_str());
        }

        void ToggleLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool ToggleLayerAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::TOGGLE_LAYER_ACTION)
                {
                        return false;
                }

                const ToggleLayerAction *o = static_cast<const ToggleLayerAction *>(&other);
                return target_layer() == o->target_layer();
        }

        void StringTyperAction::Print() const
        {
                printf("StringTyperAction\n");
        }

        void StringTyperAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool StringTyperAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::STRING_TYPER_ACTION)
                {
                        return false;
                }

                const StringTyperAction *o = static_cast<const StringTyperAction *>(&other);
                return payload() == o->payload() && keystroke_delay() == o->keystroke_delay() && repeat_delay() == o->repeat_delay();
        }

        void NonRepeatingStringTyperAction::Print() const
        {
                printf("NonRepeatingStringTyperAction\n");
        }

        void NonRepeatingStringTyperAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        QueueMessage msg;
                        for (const char c : payload_)
                        {
                                // TODO(fex): This is a really bad hack for this
                                unsigned char x = 0x04 + (std::toupper(c) - 'A');

                                msg.type = MessageType::PRESS;
                                msg.codes[0] = x;
                                msg.length = 1;
                                xQueueSend(queue, (void *)&msg, 10);

                                msg.type = MessageType::RELEASE;
                                msg.codes[0] = x;
                                msg.length = 1;
                                xQueueSend(queue, (void *)&msg, 10);

                                msg.type = MessageType::DELAY;
                                msg.delay = keystroke_delay_;
                                xQueueSend(queue, (void *)&msg, 10);
                        }
                }
        }

        bool NonRepeatingStringTyperAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::NON_REPEATING_STRING_TYPER_ACTION)
                {
                        return false;
                }

                const NonRepeatingStringTyperAction *o = static_cast<const NonRepeatingStringTyperAction *>(&other);
                return payload() == o->payload() && keystroke_delay() == o->keystroke_delay() && repeat_delay() == o->repeat_delay();
        }

        void ResetKeebAction::Print() const
        {
                printf("ResetKeebAction\n");
        }

        void ResetKeebAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        QueueMessage msg;
                        msg.type = MessageType::REBOOT;
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool ResetKeebAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::RESET_KEEB_ACTION;
        }

        void KeebBootloaderAction::Print() const
        {
                printf("KeebBootloaderAction\n");
        }

        void KeebBootloaderAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        QueueMessage msg;
                        msg.type = MessageType::REBOOT_BOOTLOADER;
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool KeebBootloaderAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::KEEB_BOOTLOADER_ACTION;
        }

        void ResetLayerAction::Print() const
        {
                printf("ResetLayerAction\n");
        }

        void ResetLayerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool ResetLayerAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::RESET_LAYER_ACTION;
        }

        void NothingburgerAction::Print() const
        {
                printf("NothingburgerAction\n");
        }

        void NothingburgerAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool NothingburgerAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::NOTHINGBURGER_ACTION;
        }

        void PassThroughAction::Print() const
        {
                printf("PassThroughAction\n");
        }

        void PassThroughAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool PassThroughAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::PASS_THROUGH_ACTION;
        }

        void ReloadKeymapAction::Print() const
        {
                printf("ReloadKeymapAction\n");
        }

        void ReloadKeymapAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool ReloadKeymapAction::operator==(const BoundAction &other)
        {
                return other.type() == BoundActionType::RELOAD_KEYMAP_ACTION;
        }

        void GenericMouseAction::Print() const
        {
                printf("GenericMouseAction: up_down: %d speed: %d\n", up_down_, speed_);
        }

        void GenericMouseAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
        }

        bool GenericMouseAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::GENERIC_MOUSE_ACTION)
                {
                        return false;
                }

                const GenericMouseAction *o = static_cast<const GenericMouseAction *>(&other);
                return up_down() == o->up_down() && speed() == o->speed();
        }

        void MouseScrollAction::Print() const
        {
                printf("MouseScrollAction: up_down: %d speed: %d\n", up_down_, speed_);
        }

        void MouseScrollAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        QueueMessage msg;
                        msg.type = (up_down_) ? MessageType::MOUSE_SCROLL_UP_DOWN : MessageType::MOUSE_SCROLL_LEFT_RIGHT;
                        msg.mouse_delta = speed_;
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool MouseScrollAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::MOUSE_SCROLL_ACTION)
                {
                        return false;
                }

                const MouseScrollAction *o = static_cast<const MouseScrollAction *>(&other);
                return up_down() == o->up_down() && speed() == o->speed();
        }

        void MouseMoveAction::Print() const
        {
                printf("MouseMoveAction: up_down: %d speed: %d\n", up_down_, speed_);
        }

        void MouseMoveAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                if (action == BoundActionEnqueue::DO)
                {
                        QueueMessage msg;
                        msg.type = (up_down_) ? MessageType::MOUSE_MOVE_UP_DOWN : MessageType::MOUSE_MOVE_LEFT_RIGHT;
                        msg.mouse_delta = speed_;
                        xQueueSend(queue, (void *)&msg, 10);
                }
        }

        bool MouseMoveAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::MOUSE_MOVE_ACTION)
                {
                        return false;
                }

                const MouseMoveAction *o = static_cast<const MouseMoveAction *>(&other);
                return up_down() == o->up_down() && speed() == o->speed();
        }

        void MouseClickAction::Print() const
        {
                printf("MouseClickAction: button: %d\n", button_);
        }

        void MouseClickAction::Enqueue(BoundActionEnqueue action, QueueHandle_t queue)
        {
                QueueMessage msg;
                msg.type = (action == BoundActionEnqueue::DO) ? MessageType::MOUSE_CLICK : MessageType::MOUSE_RELEASE;
                msg.mouse_click = button_;
                xQueueSend(queue, (void *)&msg, 10);
        }

        bool MouseClickAction::operator==(const BoundAction &other)
        {
                if (other.type() != BoundActionType::MOUSE_CLICK_ACTION)
                {
                        return false;
                }

                const MouseClickAction *o = static_cast<const MouseClickAction *>(&other);
                return button() == o->button();
        }
}