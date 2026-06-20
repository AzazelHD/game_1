#include "ui/DialogSystem.h"
#include "engine/input/Input.h"

// TODO: Implement start(lines, onFinish):
//   m_lines       = std::move(lines);
//   m_currentLine = 0;
//   m_active      = true;
//   m_onFinish    = onFinish;
//   Show the first line: m_box.open(/* screen bounds for your dialog area */);
//                        m_box.setText(m_lines[0].text);

// TODO: Implement update(float dt, const Input& input):
//   if (!m_active) return;
//   m_box.update(dt);
//
//   if (input.isKeyPressed(KeyCode::DialogAdvance))
//   {
//       if (!m_box.isFinished())
//           m_box.skipToEnd();
//       else
//       {
//           m_currentLine++;
//           if (m_currentLine >= static_cast<int>(m_lines.size()))
//           {
//               m_box.close();
//               m_active = false;
//               if (m_onFinish) m_onFinish();
//           }
//           else
//               m_box.setText(m_lines[m_currentLine].text);
//       }
//   }

// TODO: Implement render():
//   if (!m_active) return;
//   m_box.render();
//   // TODO Phase 12: draw portrait rect on the left side of the box.
//   // TODO Phase 12: draw speaker name text above the box using SDL_ttf.
