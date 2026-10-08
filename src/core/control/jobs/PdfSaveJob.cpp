#include "PdfSaveJob.h"

#include <memory>  // for unique_ptr
#include <utility>  // for move

#include <glib.h>  // for g_warning

#include "control/Control.h"               // for Control
#include "control/jobs/SaveJob.h"          // for SaveJob::updatePreview
#include "model/Document.h"                // for Document
#include "pdf/base/XojPdfExport.h"         // for XojPdfExport
#include "pdf/base/XojPdfExportFactory.h"  // for XojPdfExportFactory
#include "util/PathUtil.h"                 // for safeRenameFile
#include "util/XojMsgBox.h"                // for XojMsgBox
#include "util/i18n.h"                     // for _, FS, _F

#include "filesystem.h"  // for path, filesystem_error, remove

PdfSaveJob::PdfSaveJob(Control* control, fs::path targetPath, std::function<void(bool)> callback):
        BlockingJob(control, _("Save")), targetPath(std::move(targetPath)), callback(std::move(callback)) {}

PdfSaveJob::~PdfSaveJob() = default;

void PdfSaveJob::run() {
    SaveJob::updatePreview(control);

    Document* doc = control->getDocument();

    doc->lock_shared();
    std::unique_ptr<XojPdfExport> pdfe = XojPdfExportFactory::createExport(doc, control);
    doc->unlock_shared();

    // Render to a sibling temp file first: the target may be the very PDF this
    // document is annotating, and writing a PDF while reading that same file
    // as its background corrupts it.
    fs::path tmpPath = fs::path(this->targetPath) += ".tmp";

    if (!pdfe->createPdf(tmpPath, false)) {
        this->lastError = pdfe->getLastError();
        try {
            fs::remove(tmpPath);
        } catch (const fs::filesystem_error& fe) { g_warning("Could not remove temp save file: %s", fe.what()); }
        if (control->getWindow()) {
            callAfterRun();
        }
        return;
    }

    try {
        Util::safeRenameFile(tmpPath, this->targetPath);
    } catch (const fs::filesystem_error& fe) {
        this->lastError = FS(_F("Save file error: {1}") % fe.what());
        if (control->getWindow()) {
            callAfterRun();
        }
        return;
    }

    doc->lock();
    doc->setFilepath(this->targetPath);
    doc->unlock();

    if (control->getWindow()) {
        callAfterRun();
    }
}

void PdfSaveJob::afterRun() {
    if (!this->lastError.empty()) {
        XojMsgBox::showErrorToUser(control->getGtkWindow(), this->lastError);
        callback(false);
    } else {
        this->control->resetSavedStatus();
        callback(true);
    }
}
