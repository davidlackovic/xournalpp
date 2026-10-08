/*
 * Xournal++
 *
 * A job which saves a Document as a flattened PDF, silently overwriting the
 * document's current filepath (no dialog, no .xopp sidecar).
 *
 * @author Xournal++ Team
 * https://github.com/xournalpp/xournalpp
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <functional>
#include <string>

#include "BlockingJob.h"  // for BlockingJob
#include "filesystem.h"   // for path

class Control;

class PdfSaveJob: public BlockingJob {
public:
    PdfSaveJob(Control* control, fs::path targetPath, std::function<void(bool)> = [](bool) {});

protected:
    ~PdfSaveJob() override;

public:
    void run() override;

protected:
    void afterRun() override;

private:
    fs::path targetPath;
    std::string lastError;
    /// Called after saving, with boolean parameter true on success, false on failure (error)
    std::function<void(bool)> callback;
};
