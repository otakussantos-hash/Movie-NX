#include <algorithm>
#include <borealis/core/thread.hpp>
#include <borealis/core/touch/tap_gesture.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/image.hpp>
#include <borealis.hpp>

#include "fragment/netflix_home.hpp"
#include "config/catalog_config.h"
#include "utils/activity_helper.hpp"
#include "utils/image_helper.hpp"

namespace {

constexpr float POSTER_W    = 190;
constexpr float POSTER_H    = 285;
constexpr float POSTER_GAP  = 12;
constexpr size_t PER_ROW    = 5;   // posters por linha
constexpr size_t TOP_NEW    = 5;   // quantos "novos" aparecem no topo
constexpr float HERO_H      = 320;

const NVGcolor COLOR_BG     = nvgRGB(10, 10, 10);
const NVGcolor COLOR_RED    = nvgRGB(229, 9, 20);
const NVGcolor COLOR_WHITE  = nvgRGB(255, 255, 255);
const NVGcolor COLOR_GREY   = nvgRGB(179, 179, 179);
const NVGcolor COLOR_CARD   = nvgRGB(38, 38, 38);

// Corta o texto sem quebrar caracteres UTF-8.
std::string truncate(const std::string& s, size_t maxBytes) {
    if (s.size() <= maxBytes) return s;
    size_t n = maxBytes;
    while (n > 0 && (static_cast<unsigned char>(s[n]) & 0xC0) == 0x80) n--;
    return s.substr(0, n) + "...";
}

brls::Label* makeLabel(const std::string& text, float size, NVGcolor color) {
    auto* l = new brls::Label();
    l->setText(text);
    l->setFontSize(size);
    l->setTextColor(color);
    return l;
}

}  // namespace

NetflixHome::NetflixHome() {
    alive = std::make_shared<std::atomic<bool>>(true);
    this->setAxis(brls::Axis::COLUMN);
    this->setWidthPercentage(100);
    this->setHeightPercentage(100);
    this->setBackgroundColor(COLOR_BG);

    this->registerTabAction("Atualizar", brls::ControllerButton::BUTTON_X, [this](brls::View*) {
        this->loadCatalog();
        return true;
    });

    this->loadCatalog();
}

NetflixHome::~NetflixHome() { alive->store(false); }

void NetflixHome::onCreate() {}

brls::View* NetflixHome::create() { return new NetflixHome(); }

void NetflixHome::showMessage(const std::string& text) {
    this->clearViews();
    scroll = nullptr;
    content = nullptr;
    heroTitle = heroMeta = heroDesc = nullptr;
    heroImage = nullptr;

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setGrow(1);
    box->setJustifyContent(brls::JustifyContent::CENTER);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->addView(makeLabel(text, 22, COLOR_WHITE));
    this->addView(box);
}

void NetflixHome::loadCatalog() {
    showMessage("Carregando filmes...");

    auto flag = alive;
    brls::Threading::async([this, flag]() {
        std::vector<Movie> list;
        std::string error;
        bool ok = MovieCatalog::fetch(CATALOG_URL, list, error);
        if (ok) MovieCatalog::sortNewestFirst(list);

        brls::Threading::sync([this, flag, ok, list = std::move(list), error]() mutable {
            if (!flag->load()) return;  // tela foi destruida
            if (!ok) {
                showMessage("Nao foi possivel carregar o catalogo: " + error + "  (X = tentar de novo)");
                return;
            }
            if (list.empty()) {
                showMessage("Catalogo vazio. Adicione filmes no catalogo.json  (X = atualizar)");
                return;
            }
            this->movies = std::move(list);
            this->buildUI();
        });
    });
}

brls::Box* NetflixHome::makePoster(size_t idx) {
    const Movie& m = movies[idx];

    auto* poster = new brls::Box(brls::Axis::COLUMN);
    poster->setWidth(POSTER_W);
    poster->setHeight(POSTER_H);
    poster->setMarginRight(POSTER_GAP);
    poster->setMarginBottom(POSTER_GAP);
    poster->setBackgroundColor(COLOR_CARD);
    poster->setCornerRadius(6);
    poster->setHighlightCornerRadius(8);
    poster->setFocusable(true);
    poster->setJustifyContent(brls::JustifyContent::CENTER);
    poster->setAlignItems(brls::AlignItems::CENTER);

    // titulo de reserva (aparece atras da capa; fica visivel se a capa nao carregar)
    auto* fallback = makeLabel(truncate(m.title, 40), 16, COLOR_WHITE);
    fallback->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    fallback->setMargins(0, 8, 0, 8);
    poster->addView(fallback);

    if (!m.cover.empty()) {
        auto* img = new brls::Image();
        img->setPositionType(brls::PositionType::ABSOLUTE);
        img->setPositionTop(0);
        img->setPositionLeft(0);
        img->setWidth(POSTER_W);
        img->setHeight(POSTER_H);
        img->setScalingType(brls::ImageScalingType::FILL);
        img->setCornerRadius(6);
        poster->addView(img);
        ImageHelper::with(img)->load(m.cover);
    }

    poster->getFocusEvent()->subscribe([this, idx](bool focused) {
        if (focused) {
            heroIndex = idx;
            setHero(movies[idx]);
        }
    });
    poster->registerClickAction([this, idx](brls::View*) {
        play(idx);
        return true;
    });
    poster->addGestureRecognizer(new brls::TapGestureRecognizer(poster));
    return poster;
}

brls::Box* NetflixHome::makeRow(const std::string& title, size_t from, size_t count, brls::Box* parent) {
    auto* section = new brls::Box(brls::Axis::COLUMN);
    section->setMarginBottom(10);

    auto* lbl = makeLabel(title, 26, COLOR_WHITE);
    lbl->setMarginBottom(12);
    section->addView(lbl);

    size_t end = std::min(from + count, movies.size());
    for (size_t i = from; i < end; i += PER_ROW) {
        auto* row = new brls::Box(brls::Axis::ROW);
        for (size_t j = i; j < std::min(i + PER_ROW, end); j++) row->addView(makePoster(j));
        section->addView(row);
    }
    parent->addView(section);
    return section;
}

void NetflixHome::setHero(const Movie& m) {
    if (!heroTitle) return;
    heroTitle->setText(truncate(m.title, 60));
    std::string meta = m.year;
    if (!m.genre.empty()) meta += (meta.empty() ? "" : "  |  ") + m.genre;
    heroMeta->setText(meta);
    heroDesc->setText(truncate(m.description, 240));
    if (heroImage && !m.backdrop.empty()) ImageHelper::with(heroImage)->load(m.backdrop);
}

void NetflixHome::play(size_t idx) {
    // Passa a lista inteira: o player ja suporta barra de progresso e retomar de onde parou.
    std::vector<tsvitch::LiveM3u8> channels;
    channels.reserve(movies.size());
    for (const auto& m : movies) channels.push_back(m.toChannel());
    Intent::openLive(channels, idx, []() {});
}

void NetflixHome::buildUI() {
    this->clearViews();

    scroll = new brls::ScrollingFrame();
    scroll->setWidthPercentage(100);
    scroll->setHeightPercentage(100);

    content = new brls::Box(brls::Axis::COLUMN);
    content->setBackgroundColor(COLOR_BG);

    // ---------- HERO (banner) ----------
    auto* hero = new brls::Box(brls::Axis::ROW);
    hero->setHeight(HERO_H);
    hero->setBackgroundColor(COLOR_BG);

    heroImage = new brls::Image();
    heroImage->setPositionType(brls::PositionType::ABSOLUTE);
    heroImage->setPositionTop(0);
    heroImage->setPositionRight(0);
    heroImage->setWidthPercentage(65);
    heroImage->setHeight(HERO_H);
    heroImage->setScalingType(brls::ImageScalingType::FILL);
    hero->addView(heroImage);

    // escurece a esquerda para o texto ficar legivel (efeito Netflix)
    auto* shade = new brls::Box(brls::Axis::ROW);
    shade->setPositionType(brls::PositionType::ABSOLUTE);
    shade->setPositionTop(0);
    shade->setPositionLeft(0);
    shade->setWidthPercentage(60);
    shade->setHeight(HERO_H);
    shade->setBackgroundColor(nvgRGBA(10, 10, 10, 215));
    hero->addView(shade);

    auto* info = new brls::Box(brls::Axis::COLUMN);
    info->setWidth(520);
    info->setMargins(0, 0, 0, 30);
    info->setJustifyContent(brls::JustifyContent::CENTER);

    heroMeta  = makeLabel("", 16, COLOR_GREY);
    heroTitle = makeLabel("", 40, COLOR_WHITE);
    heroTitle->setSingleLine(true);
    heroTitle->setMarginBottom(6);
    heroDesc = makeLabel("", 16, COLOR_WHITE);
    heroDesc->setMarginBottom(16);
    heroDesc->setMarginTop(8);

    auto* playBtn = new brls::Box(brls::Axis::ROW);
    playBtn->setWidth(160);
    playBtn->setHeight(46);
    playBtn->setCornerRadius(23);
    playBtn->setHighlightCornerRadius(25);
    playBtn->setBackgroundColor(COLOR_RED);
    playBtn->setJustifyContent(brls::JustifyContent::CENTER);
    playBtn->setAlignItems(brls::AlignItems::CENTER);
    playBtn->setFocusable(true);
    playBtn->addView(makeLabel("Assistir", 20, COLOR_WHITE));
    playBtn->registerClickAction([this](brls::View*) {
        play(heroIndex);
        return true;
    });
    playBtn->addGestureRecognizer(new brls::TapGestureRecognizer(playBtn));

    info->addView(heroMeta);
    info->addView(heroTitle);
    info->addView(heroDesc);
    info->addView(playBtn);
    hero->addView(info);
    content->addView(hero);

    // ---------- LINHAS ----------
    auto* rows = new brls::Box(brls::Axis::COLUMN);
    rows->setMargins(10, 30, 30, 30);

    makeRow("Novos adicionados", 0, std::min(TOP_NEW, movies.size()), rows);
    makeRow("Todos os filmes", 0, movies.size(), rows);
    content->addView(rows);

    scroll->setContentView(content);
    this->addView(scroll);

    heroIndex = 0;
    setHero(movies[0]);

}
