#pragma once

#include <memory>
#include <atomic>
#include <vector>

#include "view/auto_tab_frame.hpp"
#include "core/MovieCatalog.hpp"

namespace brls {
class Label;
class Image;
class Box;
class ScrollingFrame;
}  // namespace brls

class NetflixHome : public AttachedView {
public:
    NetflixHome();
    ~NetflixHome() override;

    void onCreate() override;

    static brls::View* create();

private:
    void loadCatalog();
    void buildUI();
    void showMessage(const std::string& text);
    void setHero(const Movie& m);
    void play(size_t indexInMovies);

    brls::Box* makePoster(size_t indexInMovies);
    brls::Box* makeRow(const std::string& title, size_t from, size_t count, brls::Box* parent);

    std::vector<Movie> movies;  // ja ordenados: mais novo primeiro
    std::shared_ptr<std::atomic<bool>> alive;

    brls::ScrollingFrame* scroll = nullptr;
    brls::Box* content           = nullptr;
    brls::Label* heroTitle       = nullptr;
    brls::Label* heroMeta        = nullptr;
    brls::Label* heroDesc        = nullptr;
    brls::Image* heroImage       = nullptr;
    size_t heroIndex             = 0;
};
