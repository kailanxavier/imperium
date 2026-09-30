#include <app/sky_layer.h>

namespace imp::app
{
	SkyLayer::SkyLayer(const sky::SkySettings& initial)
		: ILayer("Sky")
		, m_system(initial)
	{
		m_published = m_system.state();
	}

	void SkyLayer::onAttach(AppContext& ctx)
	{
		m_cvars.push(m_system.settings());
		ctx.services.provide(m_published);
		ctx.services.provide(m_system);
	}

	void SkyLayer::onDetach(AppContext& ctx)
	{
		ctx.services.remove<sky::SkyState>();
		ctx.services.remove<sky::SkySystem>();
	}

	void SkyLayer::onUpdate(AppContext& /*ctx*/, float deltaSeconds)
	{
		m_cvars.pull(m_system.settings());
		m_published = m_system.update(deltaSeconds);
		m_cvars.push(m_system.settings());
	}
}
