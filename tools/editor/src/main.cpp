#include <editor/main_window.h>

#include <QApplication>

static QPalette createPalette()
{
	QPalette pal = QApplication::palette();

	pal.setColor(QPalette::Window, QColor(10, 10, 12));
	pal.setColor(QPalette::WindowText, QColor(220, 220, 225));

	pal.setColor(QPalette::Base, QColor(14, 14, 17));
	pal.setColor(QPalette::AlternateBase, QColor(20, 20, 24));

	pal.setColor(QPalette::Text, QColor(215, 215, 220));
	pal.setColor(QPalette::PlaceholderText, QColor(100, 100, 105));

	pal.setColor(QPalette::Button, QColor(22, 22, 26));
	pal.setColor(QPalette::ButtonText, QColor(210, 210, 215));

	pal.setColor(QPalette::Light, QColor(55, 55, 60));
	pal.setColor(QPalette::Midlight, QColor(40, 40, 44));
	pal.setColor(QPalette::Mid, QColor(32, 32, 36));
	pal.setColor(QPalette::Dark, QColor(18, 18, 21));
	pal.setColor(QPalette::Shadow, QColor(5, 5, 6));

	pal.setColor(QPalette::Highlight, QColor(42, 3, 14));
	pal.setColor(QPalette::HighlightedText, QColor(245, 235, 238));

	return pal;
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	QPalette impPalette = createPalette();
	
	app.setApplicationName("Imperium Editor");
	app.setOrganizationName("Strawberry Thief");
	app.setPalette(impPalette);

	imp::editor::MainWindow window;
	window.show();

	return app.exec();
}
