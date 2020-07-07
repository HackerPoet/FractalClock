#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <ctime>
#include "HSV.h"
#include "resource.h"

static const float PI = 3.14159265359f;
static const int max_iters = 16;
static const int window_w_init = 700;
static const int window_h_init = 700;
static const sf::Color clock_face_color(255, 255, 255, 192);
static const sf::Color bgnd_color(16, 16, 16);
enum ClockType {
  MS, HMS, HM, NUM
};
static const char* clock_type_name[] = {
  "[M] Minutes, Seconds",
  "[M] Hours, Minutes, Seconds",
  "[M] Hours, Minutes",
};
static const char* realtime_name[] = {
  "[R] Timer",
  "[R] Real-time",
};
static const char* tick_name[] = {
  "[T] Smooth Time",
  "[T] Tick Time",
};
static const char* draw_branches_name[] = {
  "[B] Hide Branches",
  "[B] Draw Branches",
};
static const char* draw_clock_name[] = {
  "[C] Hide Clock",
  "[C] Draw Clock",
};

static std::vector<sf::Vertex> line_array;
static std::vector<sf::Vertex> point_array;
static std::vector<sf::Vertex> clock_face_array1;
static std::vector<sf::Vertex> clock_face_array2;
static sf::Vector2f rotH, rotM, rotS;
static float ratioH = 0.5f;
static float ratioM = std::sqrt(1.0f / 2.0f);
static float ratioS = std::sqrt(1.0f / 2.0f);
static sf::Color color_scheme[max_iters];
static bool toggle_fullscreen = false;

static ClockType clock_type = ClockType::HMS;
static bool use_realtime = true;
static bool use_tick = false;
static bool draw_branches = true;
static bool draw_clock = true;
static bool is_fullscreen = false;

struct Res {
  Res(int id) {
    HRSRC src = ::FindResource(NULL, MAKEINTRESOURCE(id), RT_RCDATA);
    ptr = ::LockResource(::LoadResource(NULL, src));
    size = (size_t)::SizeofResource(NULL, src);
  }
  void* ptr;
  size_t size;
};

void FractalIterMS(sf::Vector2f pt, sf::Vector2f dir, int depth) {
  const sf::Color& col = color_scheme[depth];
  if (depth == 0) {
    point_array.emplace_back(pt, col);
  } else {
    const sf::Vector2f dirS((dir.x*rotS.x - dir.y*rotS.y)*ratioS, (dir.y*rotS.x + dir.x*rotS.y)*ratioS);
    const sf::Vector2f dirM((dir.x*rotM.x - dir.y*rotM.y)*ratioM, (dir.y*rotM.x + dir.x*rotM.y)*ratioM);
    FractalIterMS(pt + dirS, dirS, depth - 1);
    FractalIterMS(pt + dirM, dirM, depth - 1);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirS, col);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirM, col);
  }
}

void FractalIterHMS(sf::Vector2f pt, sf::Vector2f dir, int depth) {
  const sf::Color& col = color_scheme[depth];
  if (depth == 0) {
    point_array.emplace_back(pt, col);
  } else {
    const sf::Vector2f dirS((dir.x*rotS.x - dir.y*rotS.y)*ratioS, (dir.y*rotS.x + dir.x*rotS.y)*ratioS);
    const sf::Vector2f dirM((dir.x*rotM.x - dir.y*rotM.y)*ratioM, (dir.y*rotM.x + dir.x*rotM.y)*ratioM);
    const sf::Vector2f dirH((dir.x*rotH.x - dir.y*rotH.y)*ratioH, (dir.y*rotH.x + dir.x*rotH.y)*ratioH);
    FractalIterHMS(pt + dirS, dirS, depth - 1);
    FractalIterHMS(pt + dirM, dirM, depth - 1);
    FractalIterHMS(pt + dirH, dirH, depth - 1);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirS, col);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirM, col);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirH, col);
  }
}

void FractalIterHM(sf::Vector2f pt, sf::Vector2f dir, int depth) {
  const sf::Color& col = color_scheme[depth];
  if (depth == 0) {
    point_array.emplace_back(pt, col);
  } else {
    const sf::Vector2f dirM((dir.x*rotM.x - dir.y*rotM.y)*ratioM, (dir.y*rotM.x + dir.x*rotM.y)*ratioM);
    const sf::Vector2f dirH((dir.x*rotH.x - dir.y*rotH.y)*ratioH, (dir.y*rotH.x + dir.x*rotH.y)*ratioH);
    FractalIterHM(pt + dirM, dirM, depth - 1);
    FractalIterHM(pt + dirH, dirH, depth - 1);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirM, col);
    line_array.emplace_back(pt, col);
    line_array.emplace_back(pt + dirH, col);
  }
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd) {
  //GL settings
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 0;

  //Load the font
  sf::Font font;
  Res font_res(IDR_FONT);
  font.loadFromMemory(font_res.ptr, font_res.size);

  //Setup UI elements
  sf::Text clock_num;
  clock_num.setFont(font);
  clock_num.setFillColor(clock_face_color);
  sf::Text clock_type_text;
  clock_type_text.setFont(font);
  clock_type_text.setFillColor(clock_face_color);
  clock_type_text.setCharacterSize(24);
  clock_type_text.setPosition(10, 10);
  sf::Text realtime_text;
  realtime_text.setFont(font);
  realtime_text.setFillColor(clock_face_color);
  realtime_text.setCharacterSize(24);
  realtime_text.setPosition(10, 40);
  sf::Text tick_text;
  tick_text.setFont(font);
  tick_text.setFillColor(clock_face_color);
  tick_text.setCharacterSize(24);
  tick_text.setPosition(10, 70);
  sf::Text draw_branches_text;
  draw_branches_text.setFont(font);
  draw_branches_text.setFillColor(clock_face_color);
  draw_branches_text.setCharacterSize(24);
  draw_branches_text.setPosition(10, 100);
  sf::Text draw_clock_text;
  draw_clock_text.setFont(font);
  draw_clock_text.setFillColor(clock_face_color);
  draw_clock_text.setCharacterSize(24);
  draw_clock_text.setPosition(10, 130);

  //Create the window
  sf::VideoMode screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
  sf::RenderWindow window(screenSize, "Fractal Clock", sf::Style::Resize | sf::Style::Close, settings);
  window.setFramerateLimit(60);
  window.setActive(true);
  window.requestFocus();
  sf::Clock clock;

  //Main Loop
  clock.restart();
  while (window.isOpen()) {
    sf::Event event;
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        window.close();
        break;
      } else if (event.type == sf::Event::KeyPressed) {
        const sf::Keyboard::Key keycode = event.key.code;
        if (keycode == sf::Keyboard::Escape) {
          window.close();
          break;
        } else if (keycode == sf::Keyboard::M) {
          clock_type = ClockType((clock_type + 1) % ClockType::NUM);
        } else if (keycode == sf::Keyboard::R) {
          use_realtime = !use_realtime;
          clock.restart();
        } else if (keycode == sf::Keyboard::T) {
          use_tick = !use_tick;
        } else if (keycode == sf::Keyboard::B) {
          draw_branches = !draw_branches;
        } else if (keycode == sf::Keyboard::C) {
          draw_clock = !draw_clock;
        } else if (keycode == sf::Keyboard::F11) {
          toggle_fullscreen = true;
        }
      } else if (event.type == sf::Event::Resized) {
        screenSize.width = event.size.width;
        screenSize.height = event.size.height;
        window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, (float)screenSize.width, (float)screenSize.height)));
      }
    }

    //Calculate maximum iterations
    int iters = max_iters;
    if (clock_type == ClockType::HMS) {
      iters = max_iters - 3;
    }

    //Get the time
    float cur_time = 0.0f;
    if (use_realtime) {
      FILETIME fileTime;
      SYSTEMTIME systemTime;
      SYSTEMTIME localTime;
      GetSystemTimeAsFileTime(&fileTime);
      FileTimeToSystemTime(&fileTime, &systemTime);
      SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &localTime);
      cur_time = float(localTime.wMilliseconds) / 1000.0f;
      cur_time += float(localTime.wSecond);
      cur_time += float(localTime.wMinute) * 60.0f;
      cur_time += float(localTime.wHour) * 3600.0f;
    } else {
      cur_time = clock.getElapsedTime().asSeconds();
    }
    if (use_tick) {
      static const float a = 30.0f;
      static const float b = 14.0f;
      const float x = std::fmodf(cur_time, 1.0f);
      const float y = 1.0f - std::cos(a*x)*std::exp(-b*x);
      cur_time = cur_time - x + y;
    }

    const float seconds = std::fmodf(cur_time, 60.0f) * 2.0f * PI / 60.0f;
    const float minutes = std::fmodf(cur_time, 3600.0f) * 2.0f * PI / 3600.0f;
    const float hours = std::fmodf(cur_time, 43200.0f) * 2.0f * PI / 43200.0f;

    //Update the clock
    const float ratio = std::max(std::max(ratioH, ratioM), ratioS);
    const float start_mag = std::min(screenSize.width, screenSize.height) * 0.5f * (1.0f - ratio) / ratio;

    rotH = sf::Vector2f(std::cos(hours), std::sin(hours));
    rotM = sf::Vector2f(std::cos(minutes), std::sin(minutes));
    rotS = sf::Vector2f(std::cos(seconds), std::sin(seconds));
    const sf::Vector2f pt(float(screenSize.width)*0.5f, float(screenSize.height)*0.5f);
    const sf::Vector2f dir(0.0f, -start_mag);

    //Update the clock face
    clock_face_array1.clear();
    clock_face_array2.clear();
    for (int i = 0; i < 60; ++i) {
      const float ang = float(i) * 2.0f * PI / 60.0f;
      const sf::Vector2f v(std::cos(ang), std::sin(ang));
      const bool is_hour = (i % 5 == 0);
      std::vector<sf::Vertex>& clock_face_array = (is_hour ? clock_face_array1 : clock_face_array2);
      const float inner_rad = (is_hour ? 0.9f : 0.95f);
      clock_face_array.emplace_back(pt + v * start_mag * inner_rad, clock_face_color);
      clock_face_array.emplace_back(pt + v * start_mag * 1.0f, clock_face_color);
    }

    //Update the colors
    const float r1 = std::sin(cur_time * 0.017f)*0.5f + 0.5f;
    const float r2 = std::sin(cur_time * 0.011f)*0.5f + 0.5f;
    const float r3 = std::sin(cur_time * 0.003f)*0.5f + 0.5f;
    for (int i = 0; i < iters; ++i) {
      const float a = float(i) / float(iters - 1);
      const float h = std::fmodf(r2 + 0.5f*a, 1.0f);
      const float s = 0.5f + 0.5f * r3 - 0.5f*(1.0f - a);
      const float v = 0.3f + 0.5f * r1;
      if (i == 0) {
        color_scheme[i] = FromHSV(h, 1.0f, 1.0f);
        color_scheme[i].a = 128;
      } else if (i == iters - 1 && draw_clock) {
        color_scheme[i] = clock_face_color;
      } else {
        color_scheme[i] = FromHSV(h, s, v);
        color_scheme[i].a = 255; //128;
      }
    }

    //Update the fractal
    line_array.clear();
    point_array.clear();
    if (clock_type == ClockType::HM) {
      FractalIterHM(pt, dir, iters - 1);
    } else if (clock_type == ClockType::HMS) {
      FractalIterHMS(pt, dir, iters - 1);
    } else if (clock_type == ClockType::MS) {
      FractalIterMS(pt, dir, iters - 1);
    }

    //Clear the screen
    window.clear(bgnd_color);

    //Draw the fractal branches
    if (draw_branches) {
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(2.0f);
      if (!draw_clock) {
        window.draw(line_array.data(), line_array.size(), sf::PrimitiveType::Lines);
      } else if (clock_type == ClockType::HMS) {
        window.draw(line_array.data(), line_array.size() - 6, sf::PrimitiveType::Lines);
      } else {
        window.draw(line_array.data(), line_array.size() - 4, sf::PrimitiveType::Lines);
      }
    }

    //Draw the final fractal in a brighter color
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);
    window.draw(point_array.data(), point_array.size(), sf::PrimitiveType::Points);

    //Draw the clock
    if (draw_clock) {
      //Draw the clock face lines
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(4.0f);
      window.draw(clock_face_array1.data(), clock_face_array1.size(), sf::PrimitiveType::Lines);
      glLineWidth(2.0f);
      window.draw(clock_face_array2.data(), clock_face_array2.size(), sf::PrimitiveType::Lines);
    
      //Draw the clock face numbers
      for (int i = 0; i < 12; ++i) {
        const float ang = float(i) * 2.0f * PI / 12.0f;
        const sf::Vector2f v(std::sin(ang), -std::cos(ang));
        const std::string num_str = std::to_string(i == 0 ? 12 : i);
        clock_num.setString(num_str);
        clock_num.setCharacterSize(uint32_t(start_mag * 0.18f));

        const sf::FloatRect bounds = clock_num.getLocalBounds();
        clock_num.setOrigin(bounds.width * 0.5f, bounds.height * 0.85f);
        clock_num.setPosition(pt + v * start_mag * 0.8f);
        window.draw(clock_num);
      }

      //Draw the clock hands
      if (clock_type == ClockType::HM) {
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(5.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      } else if (clock_type == ClockType::HMS) {
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 6, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(5.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      } else if (clock_type == ClockType::MS) {
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
    }

    //Draw UI elements
    clock_type_text.setString(clock_type_name[clock_type]);
    window.draw(clock_type_text);
    realtime_text.setString(realtime_name[use_realtime ? 1 : 0]);
    window.draw(realtime_text);
    tick_text.setString(tick_name[use_tick ? 1 : 0]);
    window.draw(tick_text);
    draw_branches_text.setString(draw_branches_name[draw_branches ? 1 : 0]);
    window.draw(draw_branches_text);
    draw_clock_text.setString(draw_clock_name[draw_clock ? 1 : 0]);
    window.draw(draw_clock_text);

    //Flip the screen buffer
    window.display();

    //Toggle full-screen if needed
    if (toggle_fullscreen) {
      toggle_fullscreen = false;
      is_fullscreen = !is_fullscreen;
      if (is_fullscreen) {
        window.close();
        screenSize = sf::VideoMode::getDesktopMode();
        window.create(screenSize, "Fractal Clock", sf::Style::Fullscreen, settings);
      } else {
        window.close();
        screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
        window.create(screenSize, "Fractal Clock", sf::Style::Resize | sf::Style::Close, settings);
      }
    }
  }

  return 0;
}
