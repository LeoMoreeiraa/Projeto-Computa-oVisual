/*
 * Projeto 1 - Processamento de Imagens
 * Disciplina: Computação Visual
 * Universidade Presbiteriana Mackenzie
 * Professor: André Kishimoto
 *
 * Grupo:  
 * Leonardo Moreira - 10417555 
 * Victor Maki Tarcha - 10419861,
 * Matheus Alonso Varjao - 10417888
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
 
// ─── Constantes ───────────────────────────────────────────────────────────────
 
static const int   SEC_WIN_W      = 320;   // Largura da janela secundária
static const int   SEC_WIN_H      = 520;   // Altura da janela secundária
static const int   HIST_MARGIN    = 20;   // Margem interna do histograma
static const int   HIST_H         = 180;  // Altura do gráfico do histograma
static const int   FONT_SIZE      = 14;   // Tamanho da fonte do texto
static const int   BTN_H          = 36;   // Altura do botão
static const int   BTN_W          = 200;  // Largura do botão
static const float LUM_DARK_TH    = 85.0f;  // Limiar para imagem escura
static const float LUM_BRIGHT_TH  = 170.0f; // Limiar para imagem clara
static const float STD_LOW_TH     = 40.0f;  // Limiar para baixo contraste
static const float STD_HIGH_TH    = 80.0f;  // Limiar para alto contraste
 
// ─── Cores dos estados do botão ───────────────────────────────────────────────
 
static const SDL_Color BTN_NORMAL  = {  66, 135, 245, 255 }; // Azul padrão
static const SDL_Color BTN_HOVER   = { 130, 185, 255, 255 }; // Azul claro (mouse sobre)
static const SDL_Color BTN_PRESSED = {  30,  80, 180, 255 }; // Azul escuro (clicado)
 
// ─── Funções Auxiliares ───────────────────────────────────────────────────────
 
// Garante que o valor de intensidade esteja no intervalo [0, 255]
static Uint8 clampU8(float v) {
    if (v < 0)   return 0;
    if (v > 255) return 255;
    return (Uint8)v;
}
 
// ─── Utilidades de Imagem ─────────────────────────────────────────────────────
 
// Converte uma superfície para o formato RGBA de 32 bits
static SDL_Surface* toRGBA(SDL_Surface* src) {
    return SDL_ConvertSurface(src, SDL_PIXELFORMAT_RGBA8888);
}
 
// Verifica se a imagem já está em escala de cinza (R=G=B em todos os pixels)
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
 
// Converte imagem colorida para cinza usando a fórmula de luminância [cite: 59]
static SDL_Surface* convertToGray(SDL_Surface* src) {
    SDL_Surface* rgba = toRGBA(src);
    if (!rgba) return nullptr;
    int w = rgba->w, h = rgba->h;
    Uint8* px = (Uint8*)rgba->pixels;
    for (int i = 0; i < w * h; ++i) {
        Uint8 r = px[i * 4 + 0];
        Uint8 g = px[i * 4 + 1];
        Uint8 b = px[i * 4 + 2];
        // Cálculo ponderado: 21.25% R + 71.54% G + 07.21% B [cite: 59]
        Uint8 y = clampU8(0.2125f * r + 0.7154f * g + 0.0721f * b);
        px[i * 4 + 0] = y;
        px[i * 4 + 1] = y;
        px[i * 4 + 2] = y;
    }
    return rgba;
}
 
// Conta a frequência de cada nível de cinza (0-255)
static void computeHistogram(SDL_Surface* surf, int hist[256]) {
    memset(hist, 0, 256 * sizeof(int));
    Uint8* px = (Uint8*)surf->pixels;
    int n = surf->w * surf->h;
    for (int i = 0; i < n; ++i)
        hist[px[i * 4]]++;
}
 
// Calcula a média e o desvio padrão para análise de brilho e contraste [cite: 69, 70]
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
 
// Aplica a equalização global baseada na função de distribuição cumulativa (CDF) [cite: 74]
static SDL_Surface* equalizeHistogram(SDL_Surface* gray) {
    int hist[256];
    computeHistogram(gray, hist);
    int n = gray->w * gray->h;
 
    int cdf[256] = {};
    cdf[0] = hist[0];
    for (int i = 1; i < 256; ++i) cdf[i] = cdf[i - 1] + hist[i];
 
    int cdfMin = 0;
    for (int i = 0; i < 256; ++i) { if (cdf[i] > 0) { cdfMin = cdf[i]; break; } }
 
    Uint8 lut[256]; // Tabela de consulta (Look-up Table) para mapeamento
    for (int v = 0; v < 256; ++v) {
        if (n == cdfMin)
            lut[v] = v;
        else
            lut[v] = clampU8((float)(cdf[v] - cdfMin) / (n - cdfMin) * 255.0f);
    }
 
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
 
// ─── Renderização de Texto ────────────────────────────────────────────────────
 
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

// ─── Desenho do Histograma ────────────────────────────────────────────────────
 
static void drawHistogram(SDL_Renderer* ren, const int hist[256],
                           int x, int y, int w, int h) {
    int maxVal = 1;
    for (int i = 0; i < 256; ++i) if (hist[i] > maxVal) maxVal = hist[i];
 
    float barW = (float)w / 256.0f;

// Fundo do gráfico
    SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
    SDL_FRect bg = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(ren, &bg);

 // Barras do histograma
    SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
    for (int i = 0; i < 256; ++i) {
        float barH = (float)hist[i] / maxVal * h;
        SDL_FRect bar = { x + i * barW, y + h - barH, barW, barH };
        SDL_RenderFillRect(ren, &bar);
 }
 
    // Borda do gráfico
    SDL_SetRenderDrawColor(ren, 180, 180, 180, 255);
    SDL_FRect border = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderRect(ren, &border);
}
// ─── Desenho do Botão ─────────────────────────────────────────────────────────
 
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
 
    SDL_SetRenderDrawColor(ren, 200, 220, 255, 255);
    SDL_RenderRect(ren, &rect);
 
    SDL_Color textCol = { 255, 255, 255, 255 };
    int tw = 0, th = 0;
    TTF_GetStringSize(font, label, 0, &tw, &th);
    renderText(ren, font, label, x + (w - tw) / 2, y + (h - th) / 2, textCol);
}
// ─── Estado da Aplicação ──────────────────────────────────────────────────────
 
struct App {
    SDL_Window* mainWin  = nullptr;
    SDL_Renderer* mainRen  = nullptr;
    SDL_Window* secWin   = nullptr;
    SDL_Renderer* secRen   = nullptr;
 
    SDL_Surface* graySurf = nullptr; // Superfície original em cinza
    SDL_Surface* eqSurf   = nullptr; // Superfície equalizada
    SDL_Surface* curSurf  = nullptr; // Ponteiro para a superfície atual exibida
 
    SDL_Texture* mainTex  = nullptr;
 
    TTF_Font* font     = nullptr;
 
    bool equalized = false;
    bool btnHover  = false;
    bool btnDown   = false;
 
    int   hist[256] = {};
    float histMean  = 0;
    float histStd   = 0;
 
    SDL_Rect btnRect = {};
};
 
// Atualiza a textura da janela principal com base na superfície atual
static void reloadMainTexture(App& app) {
    if (app.mainTex) SDL_DestroyTexture(app.mainTex);
    app.mainTex = SDL_CreateTextureFromSurface(app.mainRen, app.curSurf);
}
 
// Recalcula estatísticas e histograma
static void refreshStats(App& app) {
    computeHistogram(app.curSurf, app.hist);
    int n = app.curSurf->w * app.curSurf->h;
    analyzeHistogram(app.hist, n, app.histMean, app.histStd);
}
 
// Renderiza o conteúdo da janela secundária (Histograma e Info) [cite: 65]
static void renderSecondary(App& app) {
    SDL_SetRenderDrawColor(app.secRen, 45, 45, 45, 255);
    SDL_RenderClear(app.secRen);
 
    int margin = HIST_MARGIN;
    int histW  = SEC_WIN_W - 2 * margin;
    int histY  = margin + FONT_SIZE + 8;
 
    SDL_Color white = { 230, 230, 230, 255 };
    renderText(app.secRen, app.font, "Histograma", margin, margin, white);
 
    drawHistogram(app.secRen, app.hist, margin, histY, histW, HIST_H);
 
    int infoY = histY + HIST_H + 12;
 
    // Exibe classificação de brilho [cite: 69]
    std::string meanLabel = "Media: " + std::to_string((int)app.histMean);
    const char* lumClass =
        app.histMean < LUM_DARK_TH   ? "escura" :
        app.histMean < LUM_BRIGHT_TH ? "media"  : "clara";
    renderText(app.secRen, app.font,
               (meanLabel + " (" + lumClass + ")").c_str(),
               margin, infoY, white);
    infoY += FONT_SIZE + 6;
 
    // Exibe classificação de contraste [cite: 70]
    std::string stdLabel = "Desvio padrao: " + std::to_string((int)app.histStd);
    const char* contrastClass =
        app.histStd < STD_LOW_TH  ? "baixo" :
        app.histStd < STD_HIGH_TH ? "medio" : "alto";
    renderText(app.secRen, app.font,
               (stdLabel + " (contraste " + contrastClass + ")").c_str(),
               margin, infoY, white);
    infoY += FONT_SIZE + 16;
 
    std::string statusStr = std::string("Estado: ") +
        (app.equalized ? "Equalizado" : "Original (cinza)");
    SDL_Color statusCol = app.equalized ?
        SDL_Color{120, 220, 120, 255} : SDL_Color{200, 200, 100, 255};
    renderText(app.secRen, app.font, statusStr.c_str(), margin, infoY, statusCol);
    infoY += FONT_SIZE + 16;
 
    // Desenha o botão de ação [cite: 73, 76]
    int btnX = (SEC_WIN_W - BTN_W) / 2;
    app.btnRect = { btnX, infoY, BTN_W, BTN_H };
    const char* btnLabel = app.equalized ? "Ver original" : "Equalizar";
    BtnState bs = app.btnDown   ? BtnState::PRESSED :
                  app.btnHover  ? BtnState::HOVER   : BtnState::NORMAL;
    drawButton(app.secRen, app.font, btnLabel, btnX, infoY, BTN_W, BTN_H, bs);
 
    infoY += BTN_H + 16;
    SDL_Color hint = { 150, 150, 150, 255 };
    renderText(app.secRen, app.font, "Pressione S para salvar", margin, infoY, hint);
 
    SDL_RenderPresent(app.secRen);
}
 
// Renderiza a imagem na janela principal [cite: 62]
static void renderMain(App& app) {
    SDL_SetRenderDrawColor(app.mainRen, 20, 20, 20, 255);
    SDL_RenderClear(app.mainRen);
    if (app.mainTex) SDL_RenderTexture(app.mainRen, app.mainTex, nullptr, nullptr);
    SDL_RenderPresent(app.mainRen);
}
 
// Alterna entre a versão original (cinza) e a equalizada [cite: 75]
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
 
// Salva a imagem atual como output_image.png [cite: 79]
static void saveImage(App& app) {
    const char* outPath = "output_image.png";
    if (IMG_SavePNG(app.curSurf, outPath))
        SDL_Log("Imagem salva em: %s", outPath);
    else
        SDL_Log("Erro ao salvar: %s", SDL_GetError());
}
 
// ─── Função Principal ─────────────────────────────────────────────────────────
 
int main(int argc, char* argv[]) {
    // Valida argumento de linha de comando [cite: 47]
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <caminho_da_imagem>\n", argv[0]);
        return 1;
    }
 
    // Inicializa subsistemas da SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "TTF_Init falhou: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Carrega a imagem do arquivo [cite: 55]
    SDL_Surface* loaded = IMG_Load(argv[1]);
    if (!loaded) {
        fprintf(stderr, "Erro ao carregar '%s': %s\n", argv[1], SDL_GetError());
        TTF_Quit(); SDL_Quit(); return 1;
    }

    App app;

    // Detecta e converte para escala de cinza se necessário [cite: 58, 59]
    if (isGrayscale(loaded)) {
        SDL_Log("Imagem ja esta em escala de cinza.");
        app.graySurf = toRGBA(loaded);
    } else {
        SDL_Log("Convertendo para escala de cinza.");
        app.graySurf = convertToGray(loaded);
    }
    SDL_DestroySurface(loaded);
 
    if (!app.graySurf) {
        fprintf(stderr, "Falha ao processar imagem.\n");
        TTF_Quit(); SDL_Quit(); return 1;
    }
 
    app.curSurf = app.graySurf;
    refreshStats(app);
 
    int imgW = app.graySurf->w;
    int imgH = app.graySurf->h;
 
    // Cria janela principal adaptada ao tamanho da imagem [cite: 63]
    app.mainWin = SDL_CreateWindow("Processamento de Imagens", imgW, imgH,
                                   SDL_WINDOW_RESIZABLE);
    if (!app.mainWin) {
        fprintf(stderr, "Falha ao criar janela principal: %s\n", SDL_GetError());
        goto cleanup;
    }
 
    // Centraliza na tela [cite: 63]
    SDL_SetWindowPosition(app.mainWin, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
 
    app.mainRen = SDL_CreateRenderer(app.mainWin, nullptr);
    if (!app.mainRen) {
        fprintf(stderr, "Falha ao criar renderer principal: %s\n", SDL_GetError());
        goto cleanup;
    }
 
    // Cria janela secundária filha posicionada ao lado [cite: 64]
    {
        int mx, my;
        SDL_GetWindowPosition(app.mainWin, &mx, &my);
 
        SDL_PropertiesID props = SDL_CreateProperties();
        SDL_SetPointerProperty(props, SDL_PROP_WINDOW_CREATE_PARENT_POINTER, app.mainWin);
        SDL_SetStringProperty(props,  SDL_PROP_WINDOW_CREATE_TITLE_STRING,   "Histograma");
        SDL_SetNumberProperty(props,  SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,   SEC_WIN_W);
        SDL_SetNumberProperty(props,  SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,  SEC_WIN_H);
        SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_UTILITY_BOOLEAN, true);
 
        app.secWin = SDL_CreateWindowWithProperties(props);
        SDL_DestroyProperties(props);
 
        if (!app.secWin) {
            fprintf(stderr, "Falha ao criar janela secundaria: %s\n", SDL_GetError());
            goto cleanup;
        }
        SDL_SetWindowPosition(app.secWin, mx + imgW + 8, my);
    }
 
    app.secRen = SDL_CreateRenderer(app.secWin, nullptr);
    if (!app.secRen) {
        fprintf(stderr, "Falha ao criar renderer secundario: %s\n", SDL_GetError());
        goto cleanup;
    }
 
    // Carrega fonte padrão do sistema
    {
        const char* fontPaths[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/arial.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/Library/Fonts/Arial.ttf",
            "C:/Windows/Fonts/arial.ttf",
            nullptr
        };
        for (int i = 0; fontPaths[i]; ++i) {
            app.font = TTF_OpenFont(fontPaths[i], FONT_SIZE);
            if (app.font) break;
        }
        if (!app.font)
            fprintf(stderr, "Aviso: fonte TTF nao encontrada. Textos nao serao exibidos.\n");
    }
 
    reloadMainTexture(app);
 
    // Loop de eventos do programa
    {
        bool running = true;
        SDL_Event ev;
 
        while (running) {
            while (SDL_PollEvent(&ev)) {
                switch (ev.type) {
 
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    running = false;
                    break;
 
                case SDL_EVENT_KEY_DOWN:
                    if (ev.key.key == SDLK_S)      saveImage(app); // Salva com S [cite: 79]
                    if (ev.key.key == SDLK_ESCAPE)  running = false; // Fecha com ESC
                    break;
 
                case SDL_EVENT_MOUSE_MOTION:
                    // Atualiza estado de hover do botão
                    if (ev.motion.windowID == SDL_GetWindowID(app.secWin)) {
                        int mx = (int)ev.motion.x, my = (int)ev.motion.y;
                        SDL_Rect& b = app.btnRect;
                        app.btnHover = mx >= b.x && mx < b.x + b.w &&
                                       my >= b.y && my < b.y + b.h;
                    }
                    break;
 
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    // Detecta clique no botão
                    if (ev.button.windowID == SDL_GetWindowID(app.secWin) &&
                        ev.button.button == SDL_BUTTON_LEFT) {
                        int mx = (int)ev.button.x, my = (int)ev.button.y;
                        SDL_Rect& b = app.btnRect;
                        if (mx >= b.x && mx < b.x + b.w &&
                            my >= b.y && my < b.y + b.h)
                            app.btnDown = true;
                    }
                    break;
 
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    // Executa a equalização ao soltar o clique [cite: 74, 75]
                    if (ev.button.windowID == SDL_GetWindowID(app.secWin) &&
                        ev.button.button == SDL_BUTTON_LEFT) {
                        if (app.btnDown) {
                            int mx = (int)ev.button.x, my = (int)ev.button.y;
                            SDL_Rect& b = app.btnRect;
                            if (mx >= b.x && mx < b.x + b.w &&
                                my >= b.y && my < b.y + b.h)
                                toggleEqualize(app);
                            app.btnDown = false;
                        }
                    }
                    break;
 
                case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                    if (ev.window.windowID == SDL_GetWindowID(app.secWin))
                        app.btnHover = false;
                    break;
 
                default:
                    break;
                }
            }
 
            renderMain(app);
            renderSecondary(app);
            SDL_Delay(16); // Aproximadamente 60 FPS
        }
    }
 
cleanup:
    // Gerenciamento de memória: libera recursos antes de fechar [cite: 86]
    if (app.font)     TTF_CloseFont(app.font);
    if (app.mainTex)  SDL_DestroyTexture(app.mainTex);
    if (app.eqSurf)   SDL_DestroySurface(app.eqSurf);
    if (app.graySurf) SDL_DestroySurface(app.graySurf);
    if (app.secRen)   SDL_DestroyRenderer(app.secRen);
    if (app.secWin)   SDL_DestroyWindow(app.secWin);
    if (app.mainRen)  SDL_DestroyRenderer(app.mainRen);
    if (app.mainWin)  SDL_DestroyWindow(app.mainWin);
    TTF_Quit();
    SDL_Quit();
    return 0;
}




















