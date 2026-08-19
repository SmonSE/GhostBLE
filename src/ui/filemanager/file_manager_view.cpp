#include "file_manager_view.h"
#include <M5Unified.h>
#include <SD.h>

#include "infrastructure/logging/logger.h"
#include "infrastructure/ble/ble_scanner.h"
#include "ui/menu/menu_controller.h"
#include "app/context/globals.h"


namespace FileManagerView {

// ── Layout constants ──────────────────────────────────────────
static constexpr int MENU_X        = 0;
static constexpr int MENU_Y        = 0;
static constexpr int MENU_W        = 240;
static constexpr int MENU_H        = 135;
static constexpr int ROW_H         = 11;    // pixels per row
static constexpr int ROWS_VISIBLE  = 11;    // rows on screen at once
static constexpr int INDENT_W      = 8;     // sub-item indent pixels

// ── Colors (RGB565) ───────────────────────────────────────────
static constexpr uint16_t COL_CURSOR    = 0x07E0;   // green
static constexpr uint16_t COL_STATUSBAR = 0x5ACB;   // very dark for status bar

static constexpr const char* LOG_DIR = "/GhostBLE";
static constexpr int MAX_FILES = 30;

struct FileEntry {
    char name[32];
    size_t size;
};

static FileEntry files_[MAX_FILES];
static int fileCount_ = 0;
static int cursorIdx_ = 0;
static bool open_ = false;
static size_t previewOffset_ = 0;

enum class Mode { List, Confirm, Preview };
static Mode mode_ = Mode::List;

static void loadFiles() {
    fileCount_ = 0;
    File dir = SD.open(LOG_DIR);
    if (!dir || !dir.isDirectory()) return;

    File entry = dir.openNextFile();
    while (entry && fileCount_ < MAX_FILES) {
        if (!entry.isDirectory()) {
            const char* name = entry.name();

            // Ignore hidden files and macOS metadata files
            if (name[0] == '.') {
                entry.close();
                entry = dir.openNextFile();
                continue;
            }

            strncpy(files_[fileCount_].name,
                    name,
                    sizeof(files_[fileCount_].name) - 1);

            files_[fileCount_].name[sizeof(files_[fileCount_].name) - 1] = '\0';
            files_[fileCount_].size = entry.size();
            fileCount_++;
        }
        entry.close();
        entry = dir.openNextFile();
    }
    dir.close();
}

static String formatSize(size_t bytes) {
    if (bytes < 1024) return String(bytes) + "B";
    if (bytes < 1024 * 1024) return String(bytes / 1024.0f, 1) + "KB";
    return String(bytes / (1024.0f * 1024.0f), 1) + "MB";
}

static bool isLogFile(const char* name) {
    String filename = name;
    filename.toLowerCase();
    return filename.endsWith(".log");
}

static bool isProtectedFile(const char* name) {
    String filename = name;
    filename.toLowerCase();

    return filename == "xp.dat";
}

void open() {
    loadFiles();
    cursorIdx_ = 0;
    mode_ = Mode::List;
    open_ = true;
    draw();
}

void close() {
    open_ = false;
    MenuController::open();
}

bool isOpen() { return open_; }

bool isInConfirmMode() {
    return open_ && mode_ == Mode::Confirm;
}

void navigateNext() {
    if (!open_) return;

    if (mode_ == Mode::List) {
        if (fileCount_ == 0) return;

        cursorIdx_ = (cursorIdx_ + 1) % fileCount_;
        draw();
        return;
    }
    if (mode_ == Mode::Preview) {
        previewOffset_ += 8;
        draw();
        return;
    }
}

void navigatePrev() {
    if (!open_) return;

    if (mode_ == Mode::List) {
        if (fileCount_ == 0) return;

        cursorIdx_ = (cursorIdx_ - 1 + fileCount_) % fileCount_;
        draw();
        return;
    }
    if (mode_ == Mode::Preview) {
        if (previewOffset_ >= 8)
            previewOffset_ -= 8;
        else
            previewOffset_ = 0;

        draw();
        return;
    }
}

void selectCurrent() {
    if (!open_ || fileCount_ == 0) return;

    if (mode_ == Mode::List) {
        if (isProtectedFile(files_[cursorIdx_].name)) {
            // Protected file — do nothing
            return;
        }

        if (isLogFile(files_[cursorIdx_].name)) {
            previewOffset_ = 0;
            mode_ = Mode::Preview;
        } else {
            mode_ = Mode::Confirm;
        }

        draw();
        return;
    }
    
    if (mode_ == Mode::Preview) {
        mode_ = Mode::Confirm;
        draw();
        return;
    }
}

void confirmDelete() {
    if (!open_ || mode_ != Mode::Confirm || fileCount_ == 0)
        return;

    // Block file deletion while BLE scanning is active
    if (ScanContext::bleScanEnabled.load()) {
        LOG(LOG_CONTROL, "File deletion blocked while BLE scan is running");

        mode_ = Mode::List;
        draw();
        return;
    }   

    if (isProtectedFile(files_[cursorIdx_].name)) {
        LOG(LOG_CONTROL,
            "Delete blocked for protected file: " +
            String(files_[cursorIdx_].name));

        mode_ = Mode::List;
        draw();
        return;
    }

    String path = String(LOG_DIR) + "/" + files_[cursorIdx_].name;
    if (SD.remove(path)) {
        LOG(LOG_CONTROL, "Deleted log file: " + path);
    } else {
        LOG(LOG_CONTROL, "Failed to delete: " + path);
    }

    loadFiles();
    if (cursorIdx_ >= fileCount_) cursorIdx_ = fileCount_ > 0 ? fileCount_ - 1 : 0;
    mode_ = Mode::List;
    draw();
}

void cancelAction() {
    if (!open_) return;
    mode_ = Mode::List;
    draw();
}

static void drawLogPreview() {
    if (!open_ || fileCount_ == 0) return;

    String path = String(LOG_DIR) + "/" + files_[cursorIdx_].name;
    File file = SD.open(path, FILE_READ);

    M5.Lcd.fillScreen(0x0020);

    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("LOG: %s", files_[cursorIdx_].name);

    if (!file) {
        M5.Lcd.setTextColor(0xF800, 0x0020);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("Failed to open file");

        // Status bar at bottom
        M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    #if defined(HAS_KEYBOARD)
        M5.Lcd.print("esc:back");
    #else
        M5.Lcd.print("hold big:back");
    #endif
        return;
    }

    const int maxLines = 9;
    const int lineHeight = 11;

    // Skip lines before the current scroll position
    for (size_t i = 0; i < previewOffset_ && file.available(); i++) {
        file.readStringUntil('\n');
    }

    M5.Lcd.setTextColor(0xFFFF, 0x0020);

    int y = 16;
    int lineCount = 0;

    while (file.available() && lineCount < maxLines) {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.length() == 0)
            continue;

        if (line.length() > 38)
            line = line.substring(0, 38);

        M5.Lcd.setCursor(2, y);
        M5.Lcd.print(line);

        y += lineHeight;
        lineCount++;
    }

    file.close();

    // Status bar at bottom
    M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
    M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
    M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
#if defined(HAS_KEYBOARD)
    M5.Lcd.print("up/down:scroll  enter:select  esc:back");
#else
    M5.Lcd.print("blue:scroll  big:select  hold big:back");
#endif
}

void draw() {
    if (!open_) return;

    if (mode_ == Mode::Preview) {
        drawLogPreview();
        return;
    }

    M5.Lcd.fillScreen(0x0020);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(0x07E0, 0x0020);
    M5.Lcd.setCursor(4, 2);
    M5.Lcd.printf("LOG FILES (%d)", fileCount_);

    if (fileCount_ == 0) {
        M5.Lcd.setTextColor(0x07E0, 0x5ACB);
        M5.Lcd.setCursor(4, 20);
        M5.Lcd.print("No files found");
        M5.Lcd.setCursor(4, 125);

        // Status bar at bottom
        M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    #if defined(HAS_KEYBOARD)
        M5.Lcd.print("esc:back");
    #else
        M5.Lcd.print("hold blue:back");
    #endif
        return;
    }

    int shown = min(fileCount_, 6);
    int y = 16;

    for (int i = 0; i < shown; i++) {
        int idx = (cursorIdx_ + i) % fileCount_;
        bool selected = (i == 0);
        uint16_t bg = selected ? 0x0341 : 0x0020;

        M5.Lcd.fillRect(0, y, 240, 18, bg);
        M5.Lcd.setTextColor(0x07E0, bg);
        M5.Lcd.setCursor(4, y + 2);
        M5.Lcd.print(files_[idx].name);

        if (isProtectedFile(files_[idx].name)) {
            M5.Lcd.setTextColor(0xFFE0, bg);
            M5.Lcd.setCursor(4, y + 10);
            M5.Lcd.print("protected");
        } else {
            M5.Lcd.setTextColor(0x8C71, bg);
            M5.Lcd.setCursor(4, y + 10);
            M5.Lcd.print(formatSize(files_[idx].size));
        }

        y += 18;
    }

    M5.Lcd.setTextColor(0x2945, 0x0020);
    M5.Lcd.setCursor(4, 125);

    if (mode_ == Mode::Confirm) {
        M5.Lcd.fillRect(0, 100, 240, 22, 0xF800);
        M5.Lcd.setTextColor(0xFFFF, 0xF800);
        M5.Lcd.setCursor(4, 104);
        M5.Lcd.printf("Delete %s?", files_[cursorIdx_].name);
        M5.Lcd.setCursor(4, 114);
    #if defined(HAS_KEYBOARD)
        M5.Lcd.print("enter:confirm  esc:cancel");
    #else
        M5.Lcd.print("big:confirm  hold blue:cancel");
    #endif
    } else {
        // Status bar at bottom
        M5.Lcd.fillRect(0, MENU_H - ROW_H, MENU_W, ROW_H, COL_STATUSBAR);
        M5.Lcd.setTextColor(COL_CURSOR, COL_STATUSBAR);
        M5.Lcd.setCursor(2, MENU_H - ROW_H + 2);
    #if defined(HAS_KEYBOARD)
        M5.Lcd.print("next  enter:select  esc:close");
    #else
        M5.Lcd.print("blue:scroll  big:select  hold big:close");
    #endif
    }
}

} // namespace FileManagerView
