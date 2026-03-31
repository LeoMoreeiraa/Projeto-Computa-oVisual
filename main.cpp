| (Matheus Alonso Varjao) | (10417888) |
| (Victor Maki) | (10419861) |
| (Leonardo Moreira) | (10417555) |

/*
 * Projeto 1 - Processamento de Imagens
 * Disciplina: Computação Visual
 * Universidade Presbiteriana Mackenzie
 * Professor: André Kishimoto
 *
 * Grupo: (preencha com os nomes e RAs dos integrantes)
 */

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// ─── Constants ────────────────────────────────────────────────────────────────

static const int   SEC_WIN_W      = 320;
static const int   SEC_WIN_H      = 520;
static const int   HIST_MARGIN    = 20;
static const int   HIST_H         = 180;
static const int   FONT_SIZE      = 14;
static const int   BTN_H          = 36;
static const int   BTN_W          = 200;
static const float LUM_DARK_TH    = 85.0f;
static const float LUM_BRIGHT_TH  = 170.0f;
static const float STD_LOW_TH     = 40.0f;
static const float STD_HIGH_TH    = 80.0f;

// ─── Button state colours ──────────────────────────────────────────────────────

static const SDL_Color BTN_NORMAL  = {  66, 135, 245, 255 };  // blue
static const SDL_Color BTN_HOVER   = { 130, 185, 255, 255 };  // light blue
static const SDL_Color BTN_PRESSED = {  30,  80, 180, 255 };  // dark blue

// ─── Helpers ──────────────────────────────────────────────────────────────────

static Uint8 clampU8(float v) {
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (Uint8)v;
}

// ─── Image utilities ──────────────────────────────────────────────────────────

// Returns a new RGBA surface (caller must SDL_DestroySurface)
static SDL_Surface* toRGBA(SDL_Surface* src) {
    return SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA8888);
}

// Check if a surface is already grayscale
static bool isGrayscale(SDL_Surface* surf) {
    SDL_Surface* rgba = toRGBA(surf);
    if (!rgba) return false;

    bool gray = true;
    Uint8* px = (Uint8*)rgba->pixels;
    for (int i = 0; i < rgba->w * rgba->h && gray; ++i) {
        Uint8 r = px[i * 4 + 0];
        Uint8 g = px[i * 4 + 1];
        Uint8 b = px[i * 4 + 2];
        if (r != g || g != b) gray = false;
    }
    SDL_DestroySurface(rgba);
    return gray;
}

// Convert surface to grayscale using Y = 0.2125R + 0.7154G + 0.0721B
static SDL_Surface* convertToGray(SDL_Surface* src) {
    SDL_Surface* rgba = toRGBA(src);
    if (!rgba) return nullptr;

    int w = rgba->w, h = rgba->h;
    Uint8* px = (Uint8*)rgba->pixels;

    for (int i = 0; i < w * h; ++i) {
        Uint8 r = px[i * 4 + 0];
        Uint8 g = px[i * 4 + 1];
        Uint8 b = px[i * 4 + 2];
        Uint8 y = clampU8(0.2125f * r + 0.7154f * g + 0.0721f * b);
        px[i * 4 + 0] = y;
        px[i * 4 + 1] = y;
        px[i * 4 + 2] = y;
        // alpha unchanged
    }
    return rgba;  // already RGBA, modified in place
}

// Compute histogram (256 bins) from a grayscale RGBA surface
static void computeHistogram(SDL_Surface* surf, int hist[256]) {
    memset(hist, 0, 256 * sizeof(int));
    Uint8* px = (Uint8*)surf->pixels;
    int n = surf->w * surf->h;
    for (int i = 0; i < n; ++i) {
        hist[px[i * 4]] ++;   // R == G == B for gray
    }
}

// Compute mean and std-dev from histogram
static void analyzeHistogram(const int hist[256], int totalPixels,
                              float& meanOut, float& stdOut) {
    double sum = 0, sumSq = 0;
    for (int v = 0; v < 256; ++v) {
        sum   += (double)v * hist[v];
        sumSq += (double)v * v * hist[v];
    }
    meanOut = (float)(sum / totalPixels);
    float variance = (float)(sumSq / totalPixels - (sum / totalPixels) * (sum / totalPixels));
    stdOut = sqrtf(variance < 0 ? 0 : variance);
}

// Histogram equalization — returns new RGBA surface (caller must free)
static SDL_Surface* equalizeHistogram(SDL_Surface* gray) {
    int hist[256];
    computeHistogram(gray, hist);

    int n = gray->w * gray->h;
    // Build CDF
    int cdf[256] = {};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i - 1] + hist[i];

    // Find first non-zero cdf
    int cdfMin = 0;
    for (int i = 0; i < 256; ++i) { if (cdf[i] > 0) { cdfMin = cdf[i]; break; } }

    // LUT
    Uint8 lut[256];
    for (int v = 0; v < 256; ++v) {
        if (n == cdfMin)
            lut[v] = v;
        else
            lut[v] = clampU8((float)(cdf[v] - cdfMin) / (n - cdfMin) * 255.0f);
    }

    // Apply LUT
    SDL_Surface* out = toRGBA(gray);
    Uint8* px = (Uint8*)out->pixels;
    for (int i = 0; i < n; ++i) {
        Uint8 y = lut[px[i * 4]];
        px[i * 4 + 0] = y;
        px[i * 4 + 1] = y;
        px[i * 4 + 2] = y;
    }
    return out;
}

// ─── Texture helpers ──────────────────────────────────────────────────────────

static SDL_Texture* surfaceToTexture(SDL_Renderer* ren, SDL_Surface* surf) {
    return SDL_CreateTextureFromSurface(ren, surf);
}

// ─── Text rendering (SDL_ttf) ─────────────────────────────────────────────────

static void renderText(SDL_Renderer* ren, TTF_Font* font,
                       const char* text, int x, int y, SDL_Color col) {
    SDL_Surface* s = TTF_RenderText_Blended(font, text, 0, col);
    if (!s) return;
    SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
    SDL_FRect dst = { (float)x, (float)y, (float)s->w, (float)s->h };
    SDL_DestroySurface(s);
    SDL_RenderTexture(ren, t, nullptr, &dst);
    SDL_DestroyTexture(t);
}

// ─── Draw histogram bars ──────────────────────────────────────────────────────

static void drawHistogram(SDL_Renderer* ren, const int hist[256],
                           int x, int y, int w, int h) {
    int maxVal = 1;
    for (int i = 0; i < 256; ++i) if (hist[i] > maxVal) maxVal = hist[i];

    float barW = (float)w / 256.0f;

    // Background
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_FRect bg = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(ren, &bg);

    SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
    for (int i = 0; i < 256; ++i) {
        float barH = (float)hist[i] / maxVal * h;
        SDL_FRect bar = {
            x + i * barW,
            y + h - barH,
            barW,
            barH
        };
        SDL_RenderFillRect(ren, &bar);
    }
    // Border
    SDL_SetRenderDrawColor(ren, 180, 180, 180, 255);
    SDL_FRect border = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderRect(ren, &border);
}

// ─── Draw button ──────────────────────────────────────────────────────────────

enum class BtnState { NORMAL, HOVER, PRESSED };

static void drawButton(SDL_Renderer* ren, TTF_Font* font,
                       const char* label, int x, int y, int w, int h,
                       BtnState state) {
    SDL_Color bg;
    switch (state) {
        case BtnState::HOVER:   bg = BTN_HOVER;   break;
        case BtnState::PRESSED: bg = BTN_PRESSED; break;
        default:                bg = BTN_NORMAL;  break;
    }

    SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, 255);
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(ren, &rect);

    // Border
    SDL_SetRenderDrawColor(ren, 200, 220, 255, 255);
    SDL_RenderRect(ren, &rect);

    // Centered text
    SDL_Color textCol = { 255, 255, 255, 255 };
    // Measure
    int tw = 0, th = 0;
    TTF_GetStringSize(font, label, 0, &tw, &th);
    int tx = x + (w - tw) / 2;
    int ty = y + (h - th) / 2;
    renderText(ren, font, label, tx, ty, textCol);
}

// ─── Application state ────────────────────────────────────────────────────────

struct App {
    // Windows & renderers
    SDL_Window*   mainWin    = nullptr;
    SDL_Renderer* mainRen    = nullptr;
    SDL_Window*   secWin     = nullptr;
    SDL_Renderer* secRen     = nullptr;

    // Surfaces
    SDL_Surface*  graySurf   = nullptr;   // grayscale original
    SDL_Surface*  eqSurf     = nullptr;   // equalized
    SDL_Surface*  curSurf    = nullptr;   // pointer (not owned), either gray or eq

    // Textures
    SDL_Texture*  mainTex    = nullptr;

    // Font
    TTF_Font*     font       = nullptr;

    // State
    bool equalized = false;
    bool btnHover  = false;
    bool btnDown   = false;

    // Histogram + stats
    int   hist[256]   = {};
    float histMean    = 0;
    float histStd     = 0;

    // Button rect (in secondary window coords)
    SDL_Rect btnRect = {};
};

static void reloadMainTexture(App& app) {
    if (app.mainTex) SDL_DestroyTexture(app.mainTex);
    app.mainTex = surfaceToTexture(app.mainRen, app.curSurf);
}

static void refreshStats(App& app) {
    computeHistogram(app.curSurf, app.hist);
    int n = app.curSurf->w * app.curSurf->h;
    analyzeHistogram(app.hist, n, app.histMean, app.histStd);
}

static void renderSecondary(App& app) {
    SDL_SetRenderDrawColor(app.secRen, 45, 45, 45, 255);
    SDL_RenderClear(app.secRen);

    int margin = HIST_MARGIN;
    int histW  = SEC_WIN_W - 2 * margin;
    int histY  = margin + FONT_SIZE + 8;

    // Title
    SDL_Color white = { 230, 230, 230, 255 };
    renderText(app.secRen, app.font, "Histograma", margin, margin, white);

    drawHistogram(app.secRen, app.hist, margin, histY, histW, HIST_H);

    int infoY = histY + HIST_H + 12;

    // Mean
    std::string meanLabel = "Média: " + std::to_string((int)app.histMean);
    const char* lumClass =
        app.histMean < LUM_DARK_TH   ? "escura" :
        app.histMean < LUM_BRIGHT_TH ? "média"  : "clara";
    std::string lumStr = meanLabel + " (" + lumClass + ")";
    renderText(app.secRen, app.font, lumStr.c_str(), margin, infoY, white);
    infoY += FONT_SIZE + 6;

    // Std dev
    std::string stdLabel = "Desvio padrão: " + std::to_string((int)app.histStd);
    const char* contrastClass =
        app.histStd < STD_LOW_TH  ? "baixo" :
        app.histStd < STD_HIGH_TH ? "médio" : "alto";
    std::string stdStr = stdLabel + " (contraste " + contrastClass + ")";
    renderText(app.secRen, app.font, stdStr.c_str(), margin, infoY, white);
    infoY += FONT_SIZE + 16;

    // Status label
    std::string statusStr = std::string("Estado: ") +
        (app.equalized ? "Equalizado" : "Original (cinza)");
    SDL_Color statusCol = app.equalized ?
        SDL_Color{120, 220, 120, 255} : SDL_Color{200, 200, 100, 255};
    renderText(app.secRen, app.font, statusStr.c_str(), margin, infoY, statusCol);
    infoY += FONT_SIZE + 16;

    // Button
    int btnX = (SEC_WIN_W - BTN_W) / 2;
    app.btnRect = { btnX, infoY, BTN_W, BTN_H };
    const char* btnLabel = app.equalized ? "Ver original" : "Equalizar";
    BtnState bs = app.btnDown ? BtnState::PRESSED :
                  app.btnHover ? BtnState::HOVER : BtnState::NORMAL;
    drawButton(app.secRen, app.font, btnLabel,
               btnX, infoY, BTN_W, BTN_H, bs);

    // Hint
    infoY += BTN_H + 16;
    SDL_Color hint = { 150, 150, 150, 255 };
    renderText(app.secRen, app.font, "Pressione S para salvar", margin, infoY, hint);

    SDL_RenderPresent(app.secRen);
}

static void renderMain(App& app) {
    SDL_SetRenderDrawColor(app.mainRen, 20, 20, 20, 255);
    SDL_RenderClear(app.mainRen);
    if (app.mainTex) SDL_RenderTexture(app.mainRen, app.mainTex, nullptr, nullptr);
    SDL_RenderPresent(app.mainRen);
}

static void toggleEqualize(App& app) {
    app.equalized = !app.equalized;
    if (app.equalized) {
        if (!app.eqSurf) app.eqSurf = equalizeHistogram(app.graySurf);
        app.curSurf = app.eqSurf;
    } else {
        app.curSurf = app.graySurf;
    }
    refreshStats(app);
    reloadMainTexture(app);
}

static void saveImage(App& app) {
    const char* outPath = "output_image.png";
    if (IMG_SavePNG(app.curSurf, outPath)) {
        SDL_Log("Imagem salva em: %s", outPath);
    } else {
        SDL_Log("Erro ao salvar imagem: %s", SDL_GetError());
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <caminho_da_imagem>\n", argv[0]);
        return 1;
    }

    const char* imagePath = argv[1];

    // Init SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    // Init SDL_image
    if (!IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG)) {
        fprintf(stderr, "IMG_Init falhou: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Init SDL_ttf
    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init falhou: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Load image
    SDL_Surface* loaded = IMG_Load(imagePath);
    if (!loaded) {
        fprintf(stderr, "Erro ao carregar imagem '%s': %s\n", imagePath, SDL_GetError());
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    App app;

    // Convert to grayscale if needed
    if (isGrayscale(loaded)) {
        SDL_Log("Imagem já está em escala de cinza.");
        app.graySurf = toRGBA(loaded);
    } else {
        SDL_Log("Convertendo imagem colorida para escala de cinza.");
        app.graySurf = convertToGray(loaded);
    }
    SDL_DestroySurface(loaded);

    if (!app.graySurf) {
        fprintf(stderr, "Falha ao processar imagem.\n");
        TTF_Quit(); IMG_Quit(); SDL_Quit();
        return 1;
    }

    app.curSurf = app.graySurf;

    int imgW = app.graySurf->w;
    int imgH = app.graySurf->h;

    // Compute initial stats
    refreshStats(app);

    // ── Main window ───────────────────────────────────────────────────────────
    app.mainWin = SDL_CreateWindow("Processamento de Imagens",
                                   imgW, imgH,
                                   SDL_WINDOW_RESIZABLE);
    if (!app.mainWin) {
        fprintf(stderr, "Falha ao criar janela principal: %s\n", SDL_GetError());
        goto cleanup;
    }

    // Center on primary display
    {
        SDL_DisplayID disp = SDL_GetPrimaryDisplay();
        const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(disp);
        if (dm) {
            int wx = (dm->w - imgW) / 2;
            int wy = (dm->h - imgH) / 2;
            SDL_SetWindowPosition(app.mainWin, wx, wy);
        }
    }

    app.mainRen = SDL_CreateRenderer(app.mainWin, nullptr);
    if (!app.mainRen) {
        fprintf(stderr, "Falha ao criar renderer principal: %s\n", SDL_GetError());
        goto cleanup;
    }

    // ── Secondary window (child of main) ─────────────────────────────────────
    {
        int mx, my;
        SDL_GetWindowPosition(app.mainWin, &mx, &my);
        app.secWin = SDL_CreateWindow("Histograma",
                                       SEC_WIN_W, SEC_WIN_H,
                                       0);
        if (!app.secWin) {
            fprintf(stderr, "Falha ao criar janela secundária: %s\n", SDL_GetError());
            goto cleanup;
        }
        SDL_SetWindowParent(app.secWin, app.mainWin);
        SDL_SetWindowPosition(app.secWin, mx + imgW + 8, my);
    }

    app.secRen = SDL_CreateRenderer(app.secWin, nullptr);
    if (!app.secRen) {
        fprintf(stderr, "Falha ao criar renderer secundário: %s\n", SDL_GetError());
        goto cleanup;
    }

    // ── Font ──────────────────────────────────────────────────────────────────
    // Try common system font paths
    {
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/Library/Fonts/Arial.ttf",
            "C:/Windows/Fonts/arial.ttf",
            nullptr
        };
        for (int i = 0; fontPaths[i]; ++i) {
            app.font = TTF_OpenFont(fontPaths[i], FONT_SIZE);
            if (app.font) break;
        }
        if (!app.font) {
            fprintf(stderr, "Aviso: não foi possível carregar fonte TTF. Textos não serão exibidos.\n");
        }
    }

    // Initial textures
    reloadMainTexture(app);

    // ── Event loop ────────────────────────────────────────────────────────────
    {
        bool running = true;
        SDL_Event ev;

        while (running) {
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                // Window close
                else if (ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                    running = false;
                }
                // Key press (on main window)
                else if (ev.type == SDL_EVENT_KEY_DOWN) {
                    if (ev.key.key == SDLK_S) {
                        saveImage(app);
                    } else if (ev.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                }
                // Mouse motion on secondary window
                else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                    SDL_WindowID secID = SDL_GetWindowID(app.secWin);
                    if (ev.motion.windowID == secID) {
                        int mx = (int)ev.motion.x, my = (int)ev.motion.y;
                        SDL_Rect& b = app.btnRect;
                        bool over = mx >= b.x && mx < b.x + b.w &&
                                    my >= b.y && my < b.y + b.h;
                        if (over != app.btnHover) {
                            app.btnHover = over;
                        }
                    }
                }
                // Mouse button down
                else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    SDL_WindowID secID = SDL_GetWindowID(app.secWin);
                    if (ev.button.windowID == secID && ev.button.button == SDL_BUTTON_LEFT) {
                        int mx = (int)ev.button.x, my = (int)ev.button.y;
                        SDL_Rect& b = app.btnRect;
                        if (mx >= b.x && mx < b.x + b.w &&
                            my >= b.y && my < b.y + b.h) {
                            app.btnDown = true;
                        }
                    }
                }
                // Mouse button up
                else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    SDL_WindowID secID = SDL_GetWindowID(app.secWin);
                    if (ev.button.windowID == secID && ev.button.button == SDL_BUTTON_LEFT) {
                        if (app.btnDown) {
                            int mx = (int)ev.button.x, my = (int)ev.button.y;
                            SDL_Rect& b = app.btnRect;
                            if (mx >= b.x && mx < b.x + b.w &&
                                my >= b.y && my < b.y + b.h) {
                                toggleEqualize(app);
                            }
                            app.btnDown = false;
                        }
                    }
                }
            }

            renderMain(app);
            if (app.font) renderSecondary(app);
        }
    }

cleanup:
    if (app.font)    TTF_CloseFont(app.font);
    if (app.mainTex) SDL_DestroyTexture(app.mainTex);
    if (app.eqSurf)  SDL_DestroySurface(app.eqSurf);
    if (app.graySurf)SDL_DestroySurface(app.graySurf);
    if (app.secRen)  SDL_DestroyRenderer(app.secRen);
    if (app.secWin)  SDL_DestroyWindow(app.secWin);
    if (app.mainRen) SDL_DestroyRenderer(app.mainRen);
    if (app.mainWin) SDL_DestroyWindow(app.mainWin);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
