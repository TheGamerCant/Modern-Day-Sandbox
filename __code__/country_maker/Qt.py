import sys
from PySide6.QtWidgets import (
    QApplication,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QPushButton
)

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle("My App")
        self.resize(800, 600)

        central = QWidget()
        self.setCentralWidget(central)

        layout = QVBoxLayout()

        button = QPushButton("Press")
        button.clicked.connect(self.on_press)

        layout.addWidget(button)

        central.setLayout(layout)

    def on_press(self):
        print("Pressed")

app = QApplication(sys.argv)

window = MainWindow()
window.show()

sys.exit(app.exec())