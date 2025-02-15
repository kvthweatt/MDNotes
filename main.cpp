// main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QWebEngineView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QCloseEvent>
#include <cmark.h>
#include <fstream>
#include <sstream>

class MarkdownEditor : public QMainWindow {
    Q_OBJECT

private:
    QTextEdit* editor;
    QWebEngineView* preview;
    QString currentFile;
    bool isModified;

    void setupUI() {
        // Create main splitter
        QSplitter* splitter = new QSplitter(Qt::Horizontal);
        setCentralWidget(splitter);

        // Setup editor
        editor = new QTextEdit();
        editor->setFont(QFont("Monospace", 10));
        editor->setLineWrapMode(QTextEdit::NoWrap);
        connect(editor, &QTextEdit::textChanged, this, &MarkdownEditor::onTextChanged);
        splitter->addWidget(editor);

        // Setup preview
        preview = new QWebEngineView();
        preview->setMinimumWidth(200);
        splitter->addWidget(preview);

        // Set initial splitter sizes
        QList<int> sizes;
        sizes << width()/2 << width()/2;
        splitter->setSizes(sizes);

        // Create menus
        createMenus();

        // Create toolbar for word wrap toggle
        QToolBar* toolbar = addToolBar("Format");
        QAction* wrapAction = toolbar->addAction("Toggle Word Wrap");
        wrapAction->setCheckable(true);
        connect(wrapAction, &QAction::toggled, this, &MarkdownEditor::toggleWordWrap);

        // Initialize preview with empty content
        preview->setHtml("");

        // Set window properties
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

    void updatePreview() {
        QString markdown = editor->toPlainText();
        QByteArray markdownUtf8 = markdown.toUtf8();
        
        // Convert markdown to HTML using cmark
        char* html = cmark_markdown_to_html(
            markdownUtf8.constData(),
            markdownUtf8.size(),
            CMARK_OPT_DEFAULT
        );

        // Create full HTML document
        QString htmlContent = QString::fromUtf8(html);
        QString fullHtml = QString(R"(
            <!DOCTYPE html>
            <html>
            <head>
                <meta charset="UTF-8">
                <style>
                    body {
                        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
                        line-height: 1.6;
                        padding: 20px;
                        max-width: 900px;
                        margin: 0 auto;
                    }
                    pre {
                        background-color: #f6f8fa;
                        padding: 16px;
                        border-radius: 6px;
                        overflow-x: auto;
                    }
                    code {
                        font-family: 'Consolas', 'Monaco', monospace;
                    }
                    img {
                        max-width: 100%;
                    }
                </style>
            </head>
            <body>
                %1
            </body>
            </html>
        )").arg(htmlContent);

        // Update preview directly
        preview->setHtml(fullHtml);

        free(html);
    }

    bool maybeSave() {
        if (!isModified)
            return true;

        QMessageBox::StandardButton ret = QMessageBox::warning(
            this,
            "Save Changes",
            "The document has been modified.\nDo you want to save your changes?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (ret == QMessageBox::Save)
            return saveFile();
        else if (ret == QMessageBox::Cancel)
            return false;

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

private slots:
    void onTextChanged() {
        if (!editor->document()->isEmpty() || !currentFile.isEmpty()) {
            isModified = true;
            updateWindowTitle();
        }
        updatePreview();
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
                this,
                "Open Markdown File",
                QString(),
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
        if (currentFile.isEmpty()) {
            return saveFileAs();
        }

        QFile file(currentFile);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            file.write(editor->toPlainText().toUtf8());
            isModified = false;
            updateWindowTitle();
            return true;
        }

        QMessageBox::warning(
            this,
            "Save Error",
            "Failed to save file: " + file.errorString()
        );
        return false;
    }

    bool saveFileAs() {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "Save Markdown File",
            QString(),
            "Markdown Files (*.md *.markdown);;All Files (*.*)"
        );

        if (fileName.isEmpty())
            return false;

        currentFile = fileName;
        return saveFile();
    }

    void toggleWordWrap(bool wrap) {
        editor->setLineWrapMode(
            wrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap
        );
    }

protected:
    void closeEvent(QCloseEvent* event) override {
        if (maybeSave())
            event->accept();
        else
            event->ignore();
    }

public:
    MarkdownEditor(QWidget* parent = nullptr)
        : QMainWindow(parent), isModified(false)
    {
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
