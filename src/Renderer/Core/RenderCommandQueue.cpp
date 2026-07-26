#include "RenderCommandQueue.h"
#include <algorithm>

namespace lgt {

    void RenderCommandQueue::Submit(const RenderCommand& command) {
        m_Commands.push_back(command);
    }

    void RenderCommandQueue::Sort() {
        std::sort(m_Commands.begin(), m_Commands.end(), [](const RenderCommand& a, const RenderCommand& b) {
            return a.sortKey < b.sortKey;
        });
    }


    void RenderCommandQueue::Clear() {
        m_Commands.clear();
    }

}
