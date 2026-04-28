import React, { Fragment, useMemo, useRef, useState } from "react";
import {
  Activity,
  AlertCircle,
  BarChart3,
  Check,
  Clock3,
  Cpu,
  FolderOpen,
  Image,
  Loader2,
  Play,
  RefreshCw,
  SlidersHorizontal,
  Upload,
  X,
} from "lucide-react";

const API_URL = import.meta.env.VITE_API_URL ?? "http://localhost:8000/img/compiler";
const MAX_FILES = 10;
const THREAD_OPTIONS = [6, 12, 18];

const TRANSFORMATIONS = [
  { key: "grey_v", label: "Gris vertical", accent: "teal" },
  { key: "grey_h", label: "Gris horizontal", accent: "teal" },
  { key: "color_v", label: "Color vertical", accent: "coral" },
  { key: "color_h", label: "Color horizontal", accent: "coral" },
  { key: "blur_grey", label: "Blur gris", accent: "amber" },
  { key: "blur_color", label: "Blur color", accent: "amber" },
];

const initialTransforms = TRANSFORMATIONS.reduce(
  (acc, item) => ({ ...acc, [item.key]: false }),
  {},
);

function App() {
  const fileInputRef = useRef(null);
  const [files, setFiles] = useState([]);
  const [threads, setThreads] = useState(6);
  const [transforms, setTransforms] = useState(initialTransforms);
  const [kernelGrey, setKernelGrey] = useState(3);
  const [kernelColor, setKernelColor] = useState(5);
  const [status, setStatus] = useState("idle");
  const [error, setError] = useState("");
  const [lastRun, setLastRun] = useState(null);
  const [history, setHistory] = useState([]);

  const selectedTransforms = useMemo(
    () => TRANSFORMATIONS.filter((item) => transforms[item.key]),
    [transforms],
  );

  const totalSize = useMemo(
    () => files.reduce((sum, file) => sum + file.size, 0),
    [files],
  );

  function addFiles(fileList) {
    const incoming = Array.from(fileList);
    const bmpFiles = incoming.filter((file) => file.name.toLowerCase().endsWith(".bmp"));

    if (bmpFiles.length !== incoming.length) {
      setError("Solo se aceptan archivos .bmp");
    } else {
      setError("");
    }

    setFiles((current) => {
      const known = new Set(current.map((file) => `${file.name}-${file.size}-${file.lastModified}`));
      const unique = bmpFiles.filter((file) => !known.has(`${file.name}-${file.size}-${file.lastModified}`));
      const next = [...current, ...unique].slice(0, MAX_FILES);

      if (current.length + unique.length > MAX_FILES) {
        setError(`Maximo ${MAX_FILES} imagenes BMP por corrida`);
      }

      return next;
    });

    if (fileInputRef.current) {
      fileInputRef.current.value = "";
    }
  }

  function removeFile(index) {
    setFiles((current) => current.filter((_, itemIndex) => itemIndex !== index));
  }

  function toggleTransform(key) {
    setTransforms((current) => ({ ...current, [key]: !current[key] }));
  }

  function normalizeKernel(value, fallback) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed < 1) return fallback;
    return parsed % 2 === 0 ? parsed + 1 : parsed;
  }

  async function processImages(event) {
    event.preventDefault();
    setError("");

    if (files.length === 0) {
      setError("Agrega al menos una imagen BMP");
      return;
    }

    if (selectedTransforms.length === 0) {
      setError("Selecciona al menos una transformacion");
      return;
    }

    const options = {
      ...transforms,
      threads,
      kernel_grey: normalizeKernel(kernelGrey, 3),
      kernel_color: normalizeKernel(kernelColor, 5),
    };

    const body = new FormData();
    files.forEach((file) => body.append("images", file));
    body.append("options", JSON.stringify(options));

    setStatus("running");
    const clientStart = performance.now();

    try {
      const response = await fetch(API_URL, {
        method: "POST",
        body,
      });
      const data = await response.json().catch(() => ({}));

      if (!response.ok) {
        throw new Error(readApiError(data));
      }

      const clientSeconds = (performance.now() - clientStart) / 1000;
      const executionTime = Number(
        data?.metrics?.execution_time_seconds ??
          data?.output?.execution_time_seconds ??
          data?.output?.execution_time,
      );

      const run = {
        id: `${Date.now()}-${threads}`,
        images: files.length,
        threads,
        transformations: selectedTransforms.map((item) => item.label),
        kernelGrey: options.kernel_grey,
        kernelColor: options.kernel_color,
        executionTime: Number.isFinite(executionTime) ? executionTime : 0,
        clientSeconds,
        outputPath: data?.output?.path ?? "",
      };

      setLastRun(run);
      setHistory((current) => [...current, run].slice(-8));
      setStatus("done");
    } catch (apiError) {
      setError(apiError.message || "No se pudo procesar la corrida");
      setStatus("idle");
    }
  }

  function resetRun() {
    setFiles([]);
    setTransforms(initialTransforms);
    setThreads(6);
    setKernelGrey(3);
    setKernelColor(5);
    setError("");
    setLastRun(null);
    setStatus("idle");
  }

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand-mark">
          <Activity size={22} aria-hidden="true" />
        </div>
        <div>
          <h1>TLC Image Lab</h1>
          <p>Procesamiento BMP con OpenMP</p>
        </div>
      </header>

      <main className="workspace">
        <form className="processor-panel" onSubmit={processImages}>
          <section className="section-block">
            <div className="section-heading">
              <Image size={20} aria-hidden="true" />
              <h2>Imagenes BMP</h2>
              <span className="counter">{files.length}/{MAX_FILES}</span>
            </div>

            <button
              className="dropzone"
              type="button"
              onClick={() => fileInputRef.current?.click()}
              onDragOver={(event) => event.preventDefault()}
              onDrop={(event) => {
                event.preventDefault();
                addFiles(event.dataTransfer.files);
              }}
            >
              <Upload size={26} aria-hidden="true" />
              <span>Seleccionar BMP</span>
              <small>{formatBytes(totalSize)}</small>
            </button>

            <input
              ref={fileInputRef}
              className="sr-only"
              type="file"
              accept=".bmp,image/bmp"
              multiple
              onChange={(event) => addFiles(event.target.files)}
            />

            {files.length > 0 && (
              <div className="file-list" aria-label="Archivos seleccionados">
                {files.map((file, index) => (
                  <div className="file-card" key={`${file.name}-${file.size}-${file.lastModified}`}>
                    <Image size={18} aria-hidden="true" />
                    <div className="file-meta">
                      <strong>{file.name}</strong>
                      <span>{formatBytes(file.size)}</span>
                    </div>
                    <button
                      className="icon-button"
                      type="button"
                      aria-label={`Quitar ${file.name}`}
                      onClick={() => removeFile(index)}
                    >
                      <X size={18} aria-hidden="true" />
                    </button>
                  </div>
                ))}
              </div>
            )}
          </section>

          <section className="section-block">
            <div className="section-heading">
              <Cpu size={20} aria-hidden="true" />
              <h2>Hilos</h2>
            </div>
            <div className="segmented-control" role="group" aria-label="Cantidad de hilos">
              {THREAD_OPTIONS.map((option) => (
                <button
                  key={option}
                  type="button"
                  className={threads === option ? "active" : ""}
                  onClick={() => setThreads(option)}
                >
                  {option}
                </button>
              ))}
            </div>
          </section>

          <section className="section-block">
            <div className="section-heading">
              <SlidersHorizontal size={20} aria-hidden="true" />
              <h2>Transformaciones</h2>
            </div>
            <div className="transform-grid">
              {TRANSFORMATIONS.map((item) => (
                <button
                  key={item.key}
                  type="button"
                  className={`transform-tile ${item.accent} ${transforms[item.key] ? "selected" : ""}`}
                  onClick={() => toggleTransform(item.key)}
                  aria-pressed={transforms[item.key]}
                >
                  <span>{item.label}</span>
                  <Check size={17} aria-hidden="true" />
                </button>
              ))}
            </div>
          </section>

          <section className="section-block kernel-row">
            <label className={!transforms.blur_grey ? "disabled" : ""}>
              <span>Kernel gris</span>
              <input
                type="number"
                min="1"
                step="2"
                value={kernelGrey}
                disabled={!transforms.blur_grey}
                onChange={(event) => setKernelGrey(event.target.value)}
                onBlur={() => setKernelGrey(normalizeKernel(kernelGrey, 3))}
              />
            </label>
            <label className={!transforms.blur_color ? "disabled" : ""}>
              <span>Kernel color</span>
              <input
                type="number"
                min="1"
                step="2"
                value={kernelColor}
                disabled={!transforms.blur_color}
                onChange={(event) => setKernelColor(event.target.value)}
                onBlur={() => setKernelColor(normalizeKernel(kernelColor, 5))}
              />
            </label>
          </section>

          {error && (
            <div className="alert" role="alert">
              <AlertCircle size={18} aria-hidden="true" />
              <span>{error}</span>
            </div>
          )}

          <div className="action-row">
            <button className="secondary-button" type="button" onClick={resetRun}>
              <RefreshCw size={18} aria-hidden="true" />
              Limpiar
            </button>
            <button 
              className="primary-button" 
              type="submit" disabled={status === "running"} 
              style={{ color: status === "running" ? "#9ca3af": undefined}}
            >
              {status === "running" ? (
                <>
                  <Loader2 className="spin" size={18} aria-hidden="true" />
                 Procesando
                </>
              ) : (
                <>
                <Play size={18} aria-hidden="true" />
                  Procesar
                </>
              )}
            </button>
          </div>
        </form>

        <aside className="results-panel">
          <section className="section-block">
            <div className="section-heading">
              <Clock3 size={20} aria-hidden="true" />
              <h2>Metricas</h2>
            </div>
            <Metrics run={lastRun} />
          </section>

          <section className="section-block">
            <div className="section-heading">
              <BarChart3 size={20} aria-hidden="true" />
              <h2>Tiempo por corrida</h2>
            </div>
            <TimeChart history={history} />
          </section>

          {lastRun?.outputPath && (
            <section className="section-block output-path">
              <div className="section-heading">
                <FolderOpen size={20} aria-hidden="true" />
                <h2>Salida</h2>
              </div>
              <code>{lastRun.outputPath}</code>
            </section>
          )}
        </aside>
      </main>
    </div>
  );
}

function Metrics({ run }) {
  if (!run) {
    return (
      <div className="empty-state">
        <Clock3 size={24} aria-hidden="true" />
        <span>Sin corridas</span>
      </div>
    );
  }

  const items = [
    { label: "Tiempo C", value: `${run.executionTime.toFixed(6)} s` },
    { label: "Total request", value: `${run.clientSeconds.toFixed(3)} s` },
    { label: "Imagenes", value: run.images },
    { label: "Hilos", value: run.threads },
  ];

  return (
    <div className="metrics-grid">
      {items.map((item) => (
        <div className="metric-card" key={item.label}>
          <span>{item.label}</span>
          <strong>{item.value}</strong>
        </div>
      ))}
      <div className="metric-wide">
        <span>Transformaciones</span>
        <strong>{run.transformations.join(", ")}</strong>
      </div>
    </div>
  );
}

function TimeChart({ history }) {
  if (history.length === 0) {
    return (
      <div className="empty-state">
        <BarChart3 size={24} aria-hidden="true" />
        <span>Sin datos</span>
      </div>
    );
  }

  const max = Math.max(...history.map((run) => run.executionTime), 0.001);

  return (
    <div className="bar-chart" aria-label="Grafica de tiempos">
      {history.map((run, index) => {
        const width = Math.max(4, (run.executionTime / max) * 100);
        return (
          <div className="bar-row" key={run.id}>
            <div className="bar-label">
              <strong>#{index + 1}</strong>
              <span>{run.threads} hilos</span>
            </div>
            <div className="bar-track">
              <span style={{ width: `${width}%` }} />
            </div>
            <code>{run.executionTime.toFixed(4)}s</code>
          </div>
        );
      })}
    </div>
  );
}

function formatBytes(bytes) {
  if (!bytes) return "0 KB";
  const units = ["B", "KB", "MB", "GB"];
  const index = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const value = bytes / 1024 ** index;
  return `${value.toFixed(index === 0 ? 0 : 1)} ${units[index]}`;
}

function readApiError(data) {
  if (typeof data?.detail === "string") return data.detail;
  if (data?.detail?.message) return data.detail.message;
  if (data?.detail?.error) return data.detail.error;
  return "La API rechazo la solicitud";
}

export default App;
