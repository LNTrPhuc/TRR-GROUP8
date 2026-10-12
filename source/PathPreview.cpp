#include "PathPreview.h"
#include <QPainter>
#include <QFontMetricsF>
#include <QStringList>
#include <algorithm>
#include <cmath>

PathPreview::PathPreview(const DijkstraSimulator *simulator,QWidget *parent)
    : QWidget(parent,Qt::ToolTip|Qt::FramelessWindowHint),sim(simulator) {
    setObjectName("pathPreview");
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::NoFocus);
}
void PathPreview::setVertex(int vertex) {
    destination=vertex;
    const int count=static_cast<int>(sim->pathTo(vertex).size());
    const int columns=std::clamp(count,2,5);
    resize(40+columns*110,156+std::max(1,(count+columns-1)/columns)*80);
    update();
}
void PathPreview::paintEvent(QPaintEvent *) {
    if(destination<0 || destination>=static_cast<int>(sim->graph.size())) return;
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor("#cbd5e1"),1));p.setBrush(Qt::white);
    p.drawRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),12,12);
    const auto path=sim->pathTo(destination);
    const bool settled=sim->isFinished || sim->visited[destination] || destination==sim->startNode;
    const QString start=QString::fromStdString(sim->graph[sim->startNode].name);
    const QString end=QString::fromStdString(sim->graph[destination].name);
    QFont title("Segoe UI",11,QFont::DemiBold);p.setFont(title);p.setPen(QColor("#0f172a"));
    const QString heading=path.empty() && sim->isFinished?"Không có đường đi":settled?"Đường đi ngắn nhất":"Đường đi tạm thời";
    p.drawText(QRectF(18,14,width()-36,25),Qt::AlignLeft|Qt::AlignVCenter,heading);
    p.setFont(QFont("Segoe UI",10));p.setPen(QColor("#64748b"));
    p.drawText(QRectF(18,42,width()-36,24),Qt::AlignLeft|Qt::AlignVCenter,
        p.fontMetrics().elidedText(start+" → "+end,Qt::ElideMiddle,width()-36));
    const QString distance=path.empty()?QString::fromUtf8("∞"):QString::number(sim->dist[destination]);
    p.setPen(QColor("#db2777"));
    p.drawText(QRectF(18,68,width()-36,23),Qt::AlignLeft|Qt::AlignVCenter,"Tổng trọng số: "+distance);

    const int columns=std::clamp(static_cast<int>(path.size()),2,5);
    std::vector<QPointF> positions;
    for(int i=0;i<static_cast<int>(path.size());++i) {
        const int row=i/columns,offset=i%columns;
        const int column=row%2==0?offset:columns-1-offset;
        positions.emplace_back(75+column*110,128+row*80);
    }
    for(int i=1;i<static_cast<int>(path.size());++i) {
        const QPointF a=positions[i-1],b=positions[i],delta=b-a;
        const qreal length=std::hypot(delta.x(),delta.y());
        const QPointF unit=delta/length,normal(-unit.y(),unit.x());
        const auto &edges=sim->graph[path[i-1]].edges;
        const auto edge=std::find_if(edges.begin(),edges.end(),[&](const Edge &e){return e.id==sim->prevEdgeIds[path[i]];});
        if(edge==edges.end()) continue;
        const QPointF tip=b-unit*25;
        p.setPen(QPen(QColor("#db2777"),3));p.drawLine(a+unit*25,tip);
        if(edge->directed) {
            p.setBrush(QColor("#db2777"));p.setPen(Qt::NoPen);
            p.drawPolygon(QPolygonF{tip,tip-unit*9+normal*4,tip-unit*9-normal*4});
        }
        QPointF center=(a+b)/2;
        // A vertical turn leaves the distance badge and endpoint names clear.
        if(std::abs(delta.x())<1) center+=QPointF(40,0);
        const QRectF box(center.x()-20,center.y()-10,40,20);
        p.setPen(QPen(QColor("#f9a8d4"),1));p.setBrush(Qt::white);p.drawRoundedRect(box,4,4);
        p.setFont(QFont("Segoe UI",9,QFont::DemiBold));p.setPen(QColor("#475569"));
        p.drawText(box,Qt::AlignCenter,QString::number(edge->weight));
    }
    for(int i=0;i<static_cast<int>(path.size());++i) {
        const QPointF center=positions[i];
        const QColor border=i==0?QColor("#2563eb"):i==static_cast<int>(path.size())-1?QColor("#7c3aed"):QColor("#0f766e");
        p.setPen(QPen(border,2));p.setBrush(QColor("#f0fdfa"));p.drawEllipse(center,23,23);
        if(i==0 || i==static_cast<int>(path.size())-1) {
            p.setBrush(Qt::NoBrush);p.drawEllipse(center,27,27);
        }
        QFont font("Segoe UI",11,QFont::DemiBold);font.setPixelSize(15);
        const QString name=QString::fromStdString(sim->graph[path[i]].name);
        while(font.pixelSize()>1 && QFontMetricsF(font).horizontalAdvance(name)>36) font.setPixelSize(font.pixelSize()-1);
        p.setFont(font);p.setPen(QColor("#334155"));
        p.drawText(QRectF(center.x()-19,center.y()-12,38,24),Qt::AlignCenter,name);
        p.setFont(QFont("Segoe UI",8));p.setPen(QColor("#64748b"));
        const bool turn=i+1<static_cast<int>(path.size()) && (i+1)%columns==0;
        const qreal distanceX=center.x()-(turn?36:0);
        p.drawText(QRectF(distanceX-43,center.y()+29,86,18),Qt::AlignCenter,"d = "+QString::number(sim->dist[path[i]]));
    }
    if(path.empty()) {
        // Show the inspected endpoints without inventing a connecting edge.
        for(int i=0;i<2;++i) {
            const QPointF center(i==0?75:width()-75,128);
            p.setPen(QPen(QColor(i==0?"#2563eb":"#7c3aed"),2));p.setBrush(Qt::white);
            p.drawEllipse(center,23,23);p.setBrush(Qt::NoBrush);p.drawEllipse(center,27,27);
            QFont font("Segoe UI",11,QFont::DemiBold);font.setPixelSize(15);
            const QString name=i==0?start:end;
            while(font.pixelSize()>1 && QFontMetricsF(font).horizontalAdvance(name)>36) font.setPixelSize(font.pixelSize()-1);
            p.setFont(font);p.setPen(QColor("#334155"));
            p.drawText(QRectF(center.x()-19,center.y()-12,38,24),Qt::AlignCenter,name);
        }
    }
    p.setFont(QFont("Segoe UI",9));p.setPen(QColor("#64748b"));
    QString note;
    if(path.empty()) note=sim->isFinished?"Không có đường từ nguồn theo chiều cạnh.":"Chưa tìm thấy đường đi. Thuật toán chưa hoàn tất.";
    else if(!settled) note="Khoảng cách chưa được chốt; tuyến này có thể thay đổi.";
    else if(destination==sim->startNode) note="Đây là đỉnh xuất phát; khoảng cách bằng 0.";
    else {
        QStringList names;for(int v:path) names<<QString::fromStdString(sim->graph[v].name);
        note=names.join(" → ");
    }
    p.drawText(QRectF(18,height()-65,width()-36,54),Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,note);
}
