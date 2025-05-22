// main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QWebEngineView>
#include <QSplitter>
#include <QScrollBar>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QTimer>
#include <cmark.h>

class MarkdownEditor : public QMainWindow {
    Q_OBJECT

private:
    QTextEdit* editor;
    QWebEngineView* preview;
    QString currentFile;
    bool isModified;
    QToolBar* toolbar;

    void setupUI() {
        // Create main splitter
        QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
        setCentralWidget(splitter);

        // Setup editor
        editor = new QTextEdit();
        editor->setFont(QFont("Monospace", 10));
        editor->setLineWrapMode(QTextEdit::NoWrap);
        connect(editor, &QTextEdit::textChanged, this, &MarkdownEditor::updatePreview);
        connect(editor, &QTextEdit::cursorPositionChanged, this, &MarkdownEditor::syncPreviewScroll);
        splitter->addWidget(editor);

        // Setup preview (QWebEngineView)
        preview = new QWebEngineView();
        preview->setMinimumWidth(200);
        splitter->addWidget(preview);

        // Set initial splitter sizes
        QList<int> sizes;
        sizes << width()/2 << width()/2;
        splitter->setSizes(sizes);

        // Create menus and toolbar
        createMenus();
        createToolbar();

        // Initialize
        updatePreview();
        setWindowTitle("Markdown Editor");
        resize(1200, 800);
    }

    void createMenus() {
        // File menu
        QMenu* fileMenu = menuBar()->addMenu("&File");

        QAction* newAction = new QAction("&New", this);
        newAction->setShortcut(QKeySequence::New);
        connect(newAction, &QAction::triggered, this, &MarkdownEditor::newFile);
        fileMenu->addAction(newAction);

        QAction* openAction = new QAction("&Open...", this);
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, this, &MarkdownEditor::openFile);
        fileMenu->addAction(openAction);

        fileMenu->addSeparator();

        QAction* saveAction = new QAction("&Save", this);
        saveAction->setShortcut(QKeySequence::Save);
        connect(saveAction, &QAction::triggered, this, &MarkdownEditor::saveFile);
        fileMenu->addAction(saveAction);

        QAction* saveAsAction = new QAction("Save &As...", this);
        saveAsAction->setShortcut(QKeySequence::SaveAs);
        connect(saveAsAction, &QAction::triggered, this, &MarkdownEditor::saveFileAs);
        fileMenu->addAction(saveAsAction);

        fileMenu->addSeparator();

        QAction* exitAction = new QAction("E&xit", this);
        exitAction->setShortcut(QKeySequence::Quit);
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
        fileMenu->addAction(exitAction);
    }

    void createToolbar() {
        toolbar = addToolBar("Formatting");
        QAction* wrapAction = toolbar->addAction("Word Wrap");
        wrapAction->setCheckable(true);
        connect(wrapAction, &QAction::toggled, this, &MarkdownEditor::toggleWordWrap);
    }

    void syncPreviewScroll() {
        if (!preview || !editor) return;

        // Calculate scroll percentage
        int editorPos = editor->verticalScrollBar()->value();
        int editorMax = editor->verticalScrollBar()->maximum();
        float ratio = editorMax > 0 ? (float)editorPos / editorMax : 0;

        // Apply to preview
        preview->page()->runJavaScript(
            QString("window.scrollTo(0, document.body.scrollHeight * %1);").arg(ratio)
        );
    }

    void updatePreview() {
        QString markdown = editor->toPlainText();
        QByteArray markdownUtf8 = markdown.toUtf8();
        
        char* html = cmark_markdown_to_html(
            markdownUtf8.constData(),
            markdownUtf8.size(),
            CMARK_OPT_DEFAULT
        );

        QString fullHtml = QString(R"(
            <!DOCTYPE html>
            <html>
            <head>
                <meta charset="UTF-8">
                <style>
                    body {
                        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                        line-height: 1.6;
                        padding: 20px;
                        max-width: 800px;
                        margin: 0 auto;
                        color: #333;
                    }
                    h1, h2, h3 {
                        color: #111;
                        margin-top: 1.2em;
                        margin-bottom: 0.6em;
                    }
                    pre, code {
                        font-family: 'SFMono-Regular', Consolas, 'Liberation Mono', Menlo, monospace;
                        background-color: #f6f8fa;
                        border-radius: 3px;
                    }
                    pre {
                        padding: 16px;
                        overflow: auto;
                        line-height: 1.45;
                    }
                    code {
                        padding: 0.2em 0.4em;
                        font-size: 85%;
                    }
                    blockquote {
                        border-left: 4px solid #dfe2e5;
                        color: #6a737d;
                        padding: 0 1em;
                        margin-left: 0;
                    }
                    table {
                        border-collapse: collapse;
                        width: 100%;
                    }
                    th, td {
                        border: 1px solid #dfe2e5;
                        padding: 6px 13px;
                    }
                    th {
                        background-color: #f6f8fa;
                        font-weight: 600;
                    }
                    img {
                        max-width: 100%;
                    }
                </style>
            </head>
            <body>%1</body>
            </html>
        )").arg(QString::fromUtf8(html));

        preview->setHtml(fullHtml);
        free(html);
        
        // Sync scroll position after slight delay
        QTimer::singleShot(50, this, &MarkdownEditor::syncPreviewScroll);
    }

    // [Rest of the original file handling methods remain unchanged...]
    bool maybeSave() {
        if (!isModified) return true;

        QMessageBox::StandardButton ret = QMessageBox::warning(
            this, "Save Changes",
            "The document has been modified.\nDo you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (ret == QMessageBox::Save) return saveFile();
        else if (ret == QMessageBox::Cancel) return false;
        return true;
    }

    void updateWindowTitle() {
        QString title = "Markdown Editor";
        if (!currentFile.isEmpty())
            title = QFileInfo(currentFile).fileName() + " - " + title;
        if (isModified)
            title = "*" + title;
        setWindowTitle(title);
    }

    void newFile() {
        if (maybeSave()) {
            editor->clear();
            currentFile.clear();
            isModified = false;
            updateWindowTitle();
        }
    }

    void openFile() {
        if (maybeSave()) {
            QString fileName = QFileDialog::getOpenFileName(
                this, "Open Markdown File", QString(),
                "Markdown Files (*.md *.markdown);;All Files (*.*)"
            );

            if (!fileName.isEmpty()) {
                QFile file(fileName);
                if (file.open(QFile::ReadOnly | QFile::Text)) {
                    editor->setPlainText(QString::fromUtf8(file.readAll()));
                    currentFile = fileName;
                    isModified = false;
                    updateWindowTitle();
                }
            }
        }
    }

    bool saveFile() {
        if (currentFile.isEmpty()) return saveFileAs();

        QFile file(currentFile);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            file.write(editor->toPlainText().toUtf8());
            isModified = false;
            updateWindowTitle();
            return true;
        }

        QMessageBox::warning(this, "Save Error", "Failed to save file: " + file.errorString());
        return false;
    }

    bool saveFileAs() {
        QString fileName = QFileDialog::getSaveFileName(
            this, "Save Markdown File", QString(),
            "Markdown Files (*.md *.markdown);;All Files (*.*)"
        );

        if (fileName.isEmpty()) return false;

        currentFile = fileName;
        return saveFile();
    }

    void toggleWordWrap(bool wrap) {
        editor->setLineWrapMode(wrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (maybeSave()) event->accept();
        else event->ignore();
    }

public:
    MarkdownEditor(QWidget* parent = nullptr) : QMainWindow(parent), isModified(false) {
        setupUI();
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Markdown Editor");
    QApplication::setOrganizationName("YourOrg");

    MarkdownEditor editor;
    editor.show();
    return app.exec();
}

#include "main.moc"