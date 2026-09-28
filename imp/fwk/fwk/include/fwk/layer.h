#pragma once
#include <memory>
#include <string>
#include <vector>

namespace imp::gfx { class ICommandList; }
namespace imp::app { struct AppContext; }
namespace imp::fwk
{
    class ILayer
    {
    public:
        explicit ILayer(std::string name) : m_name(std::move(name)) {}
        virtual ~ILayer() = default;

        ILayer(const ILayer&) = delete;
        ILayer& operator=(const ILayer&) = delete;

        virtual void onAttach(app::AppContext& ctx) { (void)ctx; }
        virtual void onDetach(app::AppContext& ctx) { (void)ctx; }

        virtual void onUpdate(app::AppContext& ctx, float deltaSeconds) { (void)ctx; (void)deltaSeconds; }
        virtual void onRender(app::AppContext& ctx, gfx::ICommandList& cmd) { (void)ctx; (void)cmd; }

        const std::string& name() const { return m_name; }
    private:
        std::string m_name;
    };

    class LayerStack
    {
    public:
        LayerStack() = default;
        ~LayerStack();

        LayerStack(const LayerStack&) = delete;
        LayerStack& operator=(const LayerStack&) = delete;

        void pushLayer(std::unique_ptr<ILayer> layer);
        void pushOverlay(std::unique_ptr<ILayer> overlay);

        void popLayer(const std::string& name);
        void popOverlay(const std::string& name);

        void attachPending(app::AppContext& ctx);

        void updateAll(float deltaSeconds);
        void renderAll(gfx::ICommandList& cmd);

        void clear();

        [[nodiscard]] bool attached() const { return m_ctx != nullptr; }
        [[nodiscard]] size_t layerCount() const { return m_layers.size(); }
        [[nodiscard]] size_t overlayCount() const { return m_overlays.size(); }
        [[nodiscard]] size_t pendingCount() const { return m_pending.size(); }
    private:
        struct Pending
        {
            std::unique_ptr<ILayer> layer;
            bool overlay = false;
        };

        void attachNow(std::unique_ptr<ILayer> layer, bool overlay);

        std::vector<std::unique_ptr<ILayer>> m_layers;
        std::vector<std::unique_ptr<ILayer>> m_overlays;
        std::vector<Pending> m_pending;
        app::AppContext* m_ctx = nullptr;
    };
}
