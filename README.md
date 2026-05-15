# Clasificador de residuos con visión por computadora

Sistema de detección y clasificación de objetos mediante un modelo de inteligencia artificial entrenado con Keras/TensorFlow. Utiliza la cámara web para identificar en tiempo real el tipo de residuo y mostrar la categoría predicha junto con su nivel de confianza.

En una etapa posterior, este software se comunicará con una placa **Arduino UNO** que ejecutará la separación física de la basura según la clase detectada por la IA.

## Categorías detectadas

| Clase    | Descripción            |
| -------- | ---------------------- |
| PLASTICO | Residuos plásticos     |
| METAL    | Residuos metálicos     |
| PAPEL    | Residuos de papel      |
| NADA     | Sin objeto reconocible |

Las etiquetas se definen en `model/labels.txt`.

## Requisitos

- **Python 3.11** (obligatorio; otras versiones no están soportadas para este proyecto)
- Cámara web
- macOS con Apple Silicon (opcional): el entorno incluye `tensorflow-metal` para aceleración en GPU

## Estructura del proyecto

```
proyecto_final/
├── main.py              # Script principal: captura de cámara e inferencia
├── model/
│   ├── keras_Model.h5   # Modelo entrenado (Keras)
│   └── labels.txt       # Etiquetas de las clases
├── requeriments.txt     # Dependencias con versiones fijadas
└── README.md
```

## Instalación

Se recomienda usar **Conda** para crear un entorno aislado con Python 3.11.

### 1. Crear y activar el entorno

```bash
conda create -n clasificador-basura python=3.11 -y
conda activate clasificador-basura
```

### 2. Instalar dependencias

Desde la raíz del proyecto:

```bash
pip install -r requeriments.txt
```

> **Nota:** `requeriments.txt` puede incluir una entrada de `packaging` generada por Conda con una ruta local (`file:///...`). Si `pip` falla en esa línea, instalá `packaging` por separado y volvé a instalar el resto:
>
> ```bash
> pip install packaging
> pip install -r requeriments.txt
> ```
>
> O editá temporalmente `requeriments.txt` y reemplazá esa línea por `packaging==24.2` (o la versión que indique tu entorno).

### Alternativa sin Conda

```bash
python3.11 -m venv .venv
source .venv/bin/activate   # En Windows: .venv\Scripts\activate
pip install -r requeriments.txt
```

## Uso

Con el entorno activado, ejecutá:

```bash
python main.py
```

- Se abrirá una ventana con la imagen de la cámara redimensionada a 224×224 píxeles.
- En la consola verás la **clase** predicha y el **porcentaje de confianza**.
- Para salir: tecla **Esc** o **Q**.

## Dependencias principales

Las versiones completas están en `requeriments.txt`. Entre las más relevantes:

| Paquete       | Versión  |
| ------------- | -------- |
| tensorflow    | 2.15.1   |
| keras         | 2.15.0   |
| opencv-python | 4.8.1.78 |
| numpy         | 1.26.4   |

## Roadmap

- [ ] Comunicación serial (o protocolo definido) entre Python y Arduino
- [ ] Actuación física (servos, bandejas, etc.) según la clase detectada
- [ ] Umbrales de confianza y filtrado de falsos positivos antes de enviar comandos a la placa

## Licencia y créditos

Proyecto académico — Facultad (Arduino / visión por computadora).
