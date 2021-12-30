#ifndef ACTIONS_H_
#define ACTIONS_H_

#include <memory>
#include <string>
#include <vector>

#include "FreeRTOS.h"
#include "queue.h"

#include "operation.h"

namespace fex
{
    enum class BoundActionEnqueue
    {
        DO,
        UNDO,
    };

    // This is used internally to implement operator== without rtti
    // Don't reference it, nor attempt to read type_ of any BoundAction.
    enum class BoundActionType
    {
        BOUND_ACTION,
        GENERIC_KEY_ACTION,
        PRESS_KEY_ACTION,
        RELEASE_KEY_ACTION,
        CLICK_KEY_ACTION,
        SEQUENCE_ACTION,
        DELAY_ACTION,
        GENERIC_LAYER_ACTION,
        SWITCH_TO_LAYER_ACTION,
        TEMPORARY_LAYER_ACTION,
        LEAVE_LAYER_ACTION,
        TOGGLE_LAYER_ACTION,
        STRING_TYPER_ACTION,
        NON_REPEATING_STRING_TYPER_ACTION,
        RESET_KEEB_ACTION,
        KEEB_BOOTLOADER_ACTION,
        RESET_LAYER_ACTION,
        NOTHINGBURGER_ACTION,
        PASS_THROUGH_ACTION,
        RELOAD_KEYMAP_ACTION,
        GENERIC_MOUSE_ACTION,
        MOUSE_SCROLL_ACTION,
        MOUSE_MOVE_ACTION,
        MOUSE_CLICK_ACTION,
    };

    class BoundAction
    {
    public:
        virtual ~BoundAction() = default;

        virtual void Print() const = 0;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) = 0;

        // Only to be used internally. Do not depend on an actions type.
        const BoundActionType &type() const { return type_; }

        virtual bool operator==(const BoundAction &other) = 0;

    protected:
        BoundActionType type_ = BoundActionType::BOUND_ACTION;
    };

    class GenericKeyAction : public BoundAction
    {
    public:
        GenericKeyAction(int keycode) : keycodes_({keycode}) { type_ = BoundActionType::GENERIC_KEY_ACTION; }
        GenericKeyAction(std::vector<int> keycodes) : keycodes_(std::move(keycodes)) { type_ = BoundActionType::GENERIC_KEY_ACTION; }

        GenericKeyAction(GenericKeyAction &&other) = default;
        GenericKeyAction &operator=(GenericKeyAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const std::vector<int> keycodes() const { return keycodes_; }

        virtual bool operator==(const BoundAction &other) override;

    protected:
        std::vector<int> keycodes_;
    };

    class PressKeyAction : public GenericKeyAction
    {
    public:
        PressKeyAction(int keycode) : GenericKeyAction(keycode) { type_ = BoundActionType::PRESS_KEY_ACTION; }
        PressKeyAction(std::vector<int> keycodes) : GenericKeyAction(std::move(keycodes)) { type_ = BoundActionType::PRESS_KEY_ACTION; }

        PressKeyAction(PressKeyAction &&other) = default;
        PressKeyAction &operator=(PressKeyAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ReleaseKeyAction : public GenericKeyAction
    {
    public:
        ReleaseKeyAction(int keycode) : GenericKeyAction(keycode) { type_ = BoundActionType::RELEASE_KEY_ACTION; }
        ReleaseKeyAction(std::vector<int> keycodes) : GenericKeyAction(std::move(keycodes)) { type_ = BoundActionType::RELEASE_KEY_ACTION; }

        ReleaseKeyAction(ReleaseKeyAction &&other) = default;
        ReleaseKeyAction &operator=(ReleaseKeyAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ClickKeyAction : public GenericKeyAction
    {
    public:
        ClickKeyAction(int keycode) : GenericKeyAction(keycode) { type_ = BoundActionType::CLICK_KEY_ACTION; }
        ClickKeyAction(std::vector<int> keycodes) : GenericKeyAction(std::move(keycodes)) { type_ = BoundActionType::CLICK_KEY_ACTION; }

        ClickKeyAction(ClickKeyAction &&other) = default;
        ClickKeyAction &operator=(ClickKeyAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class SequenceAction : public BoundAction
    {
    public:
        SequenceAction(std::vector<std::unique_ptr<BoundAction>> sequence) : sequence_(std::move(sequence)) { type_ = BoundActionType::SEQUENCE_ACTION; }

        SequenceAction(SequenceAction &&other) = default;
        SequenceAction &operator=(SequenceAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const std::vector<std::unique_ptr<BoundAction>> &sequence() const { return sequence_; }

        virtual bool operator==(const BoundAction &other) override;

    private:
        std::vector<std::unique_ptr<BoundAction>> sequence_;
    };

    class DelayAction : public BoundAction
    {
    public:
        DelayAction(unsigned long duration) : duration_(duration) { type_ = BoundActionType::DELAY_ACTION; }

        DelayAction(DelayAction &&other) = default;
        DelayAction &operator=(DelayAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const unsigned long duration() const { return duration_; }

        virtual bool operator==(const BoundAction &other) override;

    private:
        unsigned long duration_;
    };

    class GenericLayerAction : public BoundAction
    {
    public:
        GenericLayerAction(std::string target_layer) : target_layer_(target_layer) { type_ = BoundActionType::GENERIC_LAYER_ACTION; }

        GenericLayerAction(GenericLayerAction &&other) = default;
        GenericLayerAction &operator=(GenericLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const std::string &target_layer() const { return target_layer_; }

        virtual bool operator==(const BoundAction &other) override;

    protected:
        std::string target_layer_;
    };

    class SwitchToLayerAction : public GenericLayerAction
    {
    public:
        SwitchToLayerAction(std::string target_layer) : GenericLayerAction(target_layer) { type_ = BoundActionType::SWITCH_TO_LAYER_ACTION; }

        SwitchToLayerAction(SwitchToLayerAction &&other) = default;
        SwitchToLayerAction &operator=(SwitchToLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class TemporaryLayerAction : public GenericLayerAction
    {
    public:
        TemporaryLayerAction(std::string target_layer) : GenericLayerAction(target_layer) { type_ = BoundActionType::TEMPORARY_LAYER_ACTION; }

        TemporaryLayerAction(TemporaryLayerAction &&other) = default;
        TemporaryLayerAction &operator=(TemporaryLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class LeaveLayerAction : public GenericLayerAction
    {
    public:
        LeaveLayerAction(std::string target_layer) : GenericLayerAction(target_layer) { type_ = BoundActionType::LEAVE_LAYER_ACTION; }

        LeaveLayerAction(LeaveLayerAction &&other) = default;
        LeaveLayerAction &operator=(LeaveLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ToggleLayerAction : public GenericLayerAction
    {
    public:
        ToggleLayerAction(std::string target_layer) : GenericLayerAction(target_layer) { type_ = BoundActionType::TOGGLE_LAYER_ACTION; }

        ToggleLayerAction(ToggleLayerAction &&other) = default;
        ToggleLayerAction &operator=(ToggleLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class StringTyperAction : public BoundAction
    {
    public:
        StringTyperAction(std::string payload, unsigned long keystroke_delay, unsigned long repeat_delay)
            : payload_(payload), keystroke_delay_(keystroke_delay), repeat_delay_(repeat_delay) { type_ = BoundActionType::STRING_TYPER_ACTION; }

        StringTyperAction(StringTyperAction &&other) = default;
        StringTyperAction &operator=(StringTyperAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const std::string &payload() const { return payload_; }
        const unsigned long keystroke_delay() const { return keystroke_delay_; }
        const unsigned long repeat_delay() const { return repeat_delay_; }

        virtual bool operator==(const BoundAction &other) override;

    protected:
        std::string payload_;
        unsigned long keystroke_delay_;
        unsigned long repeat_delay_;
    };

    class NonRepeatingStringTyperAction : public StringTyperAction
    {
    public:
        NonRepeatingStringTyperAction(std::string payload, unsigned long keystroke_delay)
            : StringTyperAction(payload, keystroke_delay, 0) { type_ = BoundActionType::NON_REPEATING_STRING_TYPER_ACTION; }

        NonRepeatingStringTyperAction(NonRepeatingStringTyperAction &&other) = default;
        NonRepeatingStringTyperAction &operator=(NonRepeatingStringTyperAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ResetKeebAction : public BoundAction
    {
    public:
        ResetKeebAction() { type_ = BoundActionType::RESET_KEEB_ACTION; };

        ResetKeebAction(ResetKeebAction &&other) = default;
        ResetKeebAction &operator=(ResetKeebAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class KeebBootloaderAction : public BoundAction
    {
    public:
        KeebBootloaderAction() { type_ = BoundActionType::KEEB_BOOTLOADER_ACTION; }

        KeebBootloaderAction(KeebBootloaderAction &&other) = default;
        KeebBootloaderAction &operator=(KeebBootloaderAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ResetLayerAction : public BoundAction
    {
    public:
        ResetLayerAction() { type_ = BoundActionType::RESET_LAYER_ACTION; }

        ResetLayerAction(ResetLayerAction &&other) = default;
        ResetLayerAction &operator=(ResetLayerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class NothingburgerAction : public BoundAction
    {
    public:
        NothingburgerAction() { type_ = BoundActionType::NOTHINGBURGER_ACTION; }

        NothingburgerAction(NothingburgerAction &&other) = default;
        NothingburgerAction &operator=(NothingburgerAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class PassThroughAction : public BoundAction
    {
    public:
        PassThroughAction() { type_ = BoundActionType::PASS_THROUGH_ACTION; }

        PassThroughAction(PassThroughAction &&other) = default;
        PassThroughAction &operator=(PassThroughAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class ReloadKeymapAction : public BoundAction
    {
    public:
        ReloadKeymapAction() { type_ = BoundActionType::RELOAD_KEYMAP_ACTION; }

        ReloadKeymapAction(ReloadKeymapAction &&other) = default;
        ReloadKeymapAction &operator=(ReloadKeymapAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class GenericMouseAction : public BoundAction
    {
    public:
        GenericMouseAction(bool up_down, int8_t speed) : up_down_(up_down), speed_(speed) { type_ = BoundActionType::GENERIC_MOUSE_ACTION; }

        GenericMouseAction(GenericMouseAction &&other) = default;
        GenericMouseAction &operator=(GenericMouseAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const bool up_down() const { return up_down_; }
        const int8_t speed() const { return speed_; }

        virtual bool operator==(const BoundAction &other) override;

    protected:
        bool up_down_;
        int8_t speed_;
    };

    class MouseScrollAction : public GenericMouseAction
    {
    public:
        MouseScrollAction(bool up_down, int8_t speed) : GenericMouseAction(up_down, speed) { type_ = BoundActionType::MOUSE_SCROLL_ACTION; }

        MouseScrollAction(MouseScrollAction &&other) = default;
        MouseScrollAction &operator=(MouseScrollAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class MouseMoveAction : public GenericMouseAction
    {
    public:
        MouseMoveAction(bool up_down, int8_t speed) : GenericMouseAction(up_down, speed) { type_ = BoundActionType::MOUSE_MOVE_ACTION; }

        MouseMoveAction(MouseMoveAction &&other) = default;
        MouseMoveAction &operator=(MouseMoveAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        virtual bool operator==(const BoundAction &other) override;
    };

    class MouseClickAction : public BoundAction 
    {
    public:
        MouseClickAction(uint8_t button) : button_(button) { type_ = BoundActionType::MOUSE_CLICK_ACTION; }

        MouseClickAction(MouseClickAction &&other) = default;
        MouseClickAction &operator=(MouseClickAction &&other) = default;

        virtual void Print() const override;
        virtual void Enqueue(BoundActionEnqueue action, QueueHandle_t queue) override;

        const uint8_t button() const { return button_; }

        virtual bool operator==(const BoundAction &other) override;
    

    private:
        int8_t button_;
    };
}

#endif