#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>
#include <QLabel>
#include <QComboBox>
#include "DijkstraSimulator.h"
#include "GraphWidget.h"

class QHBoxLayout;
class QTableWidget;
class QListWidget;
class QTextEdit;
class QPushButton;
class QSlider;
class QTabWidget;
class PathPreview;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private slots:
    void onPlayPause();
    void onNextStep();
    void onPrevStep();
    void onReset();
    void onGraphTemplateChanged(int index);
    void onRandomGraph();
    void onImportText();
    void updateUI();

private:
    DijkstraSimulator* simulator;
    GraphWidget* graphWidget;

    QComboBox* comboTemplates;
    QTableWidget* distTable;
    QListWidget* pqList;
    QTextEdit* codeViewer;
    QTabWidget* detailTabs;
    QPushButton* codeToggle;

    QPushButton* btnPlay;
    QPushButton* btnNext;
    QPushButton* btnPrev;
    QSlider* speedSlider;
    QComboBox *sourceSelector, *targetSelector, *edgeKindSelector;
    PathPreview *pathPreview;
    int hoveredVertex=-1;
    QPoint hoverPosition;
    void showPathPreview(int row, QPoint globalPosition);
    void hidePathPreview();
    int playbackSteps = 1;
    double stepBudget = 0;
    QElapsedTimer playbackClock;
    double playbackDuration() const;
    void advancePlayback();

    QLabel* lblExplanation;
    QLabel* lblStatus;
    QLabel* lblStep;
    QLabel* lblNodes;
    QLabel* lblEdges;
    QLabel* lblPath;

    QTimer* playTimer;

    void setupUI();
    void setupLeftPanel(QHBoxLayout* mainLayout);
    void setupRightPanel(QHBoxLayout* mainLayout);
    void updateCodeHighlight();
    void markCustomGraph();
    void showInstantResult();
};

#endif // MAINWINDOW_H
