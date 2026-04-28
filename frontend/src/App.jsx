import React, { useMemo, useRef, useState } from "react";
import {
  AlertCircle,
  BarChart3,
  Check,
  Clock3,
  Cpu,
  Download,
  FolderOpen,
  Image,
  Loader2,
  Play,
  RefreshCw,
  SlidersHorizontal,
  Upload,
  X,
} from "lucide-react";
import tecLogo from "./assets/tec.png";

const API_URL = import.meta.env.VITE_API_URL ?? "http://localhost:8000/img/compiler";
const MAX_FILES = 10;
const THREAD_OPTIONS = [6, 12, 18];
const THREAD_MODES = {
  single: "single",
  compare: "compare",
};

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
const TEAM_MEMBERS = [
  { name: "Jonathan Armando Arredondo Hernandez", id: "A01737788" },
  { name: "Diego Javier Solórzano Trinidad", id: "A01808035" },
  { name: "Rusbel Alejandro Morales Méndez", id: "A01737814" },
  { name: "Pablo Andre Coca Murillo", id: "A01737438" },
  { name: "Fernando Maggi Llerandi", id: "A01736935" },
];

function App() {
  const fileInputRef = useRef(null);
  const [files, setFiles] = useState([]);
  const [threads, setThreads] = useState(6);
  const [threadMode, setThreadMode] = useState(THREAD_MODES.single);
  const [transforms, setTransforms] = useState(initialTransforms);
  const [kernelGrey, setKernelGrey] = useState(3);
  const [kernelColor, setKernelColor] = useState(5);
  const [status, setStatus] = useState("idle");
  const [activeThread, setActiveThread] = useState(null);
  const [downloadStatus, setDownloadStatus] = useState("idle");
  const [error, setError] = useState("");
  const [lastRun, setLastRun] = useState(null);
  const [history, setHistory] = useState([]);

  const selectedTransforms = useMemo(
    () => TRANSFORMATIONS.filter((item) => transforms[item.key]),
    [transforms],
  );
  const allTransformsSelected = selectedTransforms.length === TRANSFORMATIONS.length;

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

  function toggleAllTransforms() {
    const shouldSelect = !allTransformsSelected;
    setTransforms(
      TRANSFORMATIONS.reduce(
        (acc, item) => ({ ...acc, [item.key]: shouldSelect }),
        {},
      ),
    );
  }

  function normalizeKernel(value, fallback) {
    const parsed = Number(value);
    if (!Number.isFinite(parsed) || parsed < 1) return fallback;
    return parsed % 2 === 0 ? parsed + 1 : parsed;
  }

  function createOptions(threadCount) {
    const options = {
      ...transforms,
      threads: threadCount,
      kernel_grey: normalizeKernel(kernelGrey, 3),
      kernel_color: normalizeKernel(kernelColor, 5),
    };

    return options;
  }

  async function runCompiler(threadCount, selectedLabels) {
    const options = createOptions(threadCount);
    const body = new FormData();
    files.forEach((file) => body.append("images", file));
    body.append("options", JSON.stringify(options));

    const clientStart = performance.now();
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
    const runId = `${Date.now()}-${threadCount}`;

    return {
      id: runId,
      images: files.length,
      threads: threadCount,
      transformations: selectedLabels,
      kernelGrey: options.kernel_grey,
      kernelColor: options.kernel_color,
      executionTime: Number.isFinite(executionTime) ? executionTime : 0,
      clientSeconds,
      outputPath: data?.output?.path ?? "",
      outputImages: (data?.output?.images ?? []).map((image, index) => ({
        ...image,
        url: resolveOutputUrl(image.url, `${runId}-${index}`),
      })),
    };
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

    const threadList = threadMode === THREAD_MODES.compare ? THREAD_OPTIONS : [threads];
    const selectedLabels = selectedTransforms.map((item) => item.label);

    setStatus("running");
    setActiveThread(threadList[0]);

    try {
      const runs = [];
      for (const threadCount of threadList) {
        setActiveThread(threadCount);
        const run = await runCompiler(threadCount, selectedLabels);
        runs.push(run);
      }

      const nextRun = threadMode === THREAD_MODES.compare ? createComparisonRun(runs) : runs[0];
      setLastRun(nextRun);
      setHistory((current) => [...current, ...runs].slice(-12));
      setStatus("done");
    } catch (apiError) {
      setError(apiError.message || "No se pudo procesar la corrida");
      setStatus("idle");
    } finally {
      setActiveThread(null);
    }
  }

  async function downloadResults() {
    if (!lastRun?.outputImages?.length) {
      setError("No hay imagenes producidas para descargar");
      return;
    }

    setError("");
    setDownloadStatus("running");

    try {
      const response = await fetch(resolveBackendUrl("/img/download-results"), {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify(lastRun.outputImages.map((image) => image.filename)),
      });

      if (!response.ok) {
        const data = await response.json().catch(() => ({}));
        throw new Error(readApiError(data));
      }

      const blob = await response.blob();
      const url = URL.createObjectURL(blob);
      const link = document.createElement("a");
      link.href = url;
      link.download = `resultados_tlc_${Date.now()}.zip`;
      document.body.appendChild(link);
      link.click();
      link.remove();
      URL.revokeObjectURL(url);
      setDownloadStatus("done");
    } catch (apiError) {
      setError(apiError.message || "No se pudo descargar el resultado");
      setDownloadStatus("idle");
    }
  }

  function resetRun() {
    setFiles([]);
    setTransforms(initialTransforms);
    setThreads(6);
    setThreadMode(THREAD_MODES.single);
    setKernelGrey(3);
    setKernelColor(5);
    setError("");
    setActiveThread(null);
    setDownloadStatus("idle");
    setLastRun(null);
    setStatus("idle");
  }

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand-lockup">
          <img className="tec-logo" src={tecLogo} alt="Tecnologico de Monterrey" />
          <div>
            <h1>TLC Image Lab</h1>
            <p>Procesamiento BMP con OpenMP</p>
          </div>
        </div>

        <div className="team-strip" aria-label="Integrantes">
          {TEAM_MEMBERS.map((member) => (
            <div className="team-member" key={member.id}>
              <span>{member.name}</span>
              <strong>{member.id}</strong>
            </div>
          ))}
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
            <div className="mode-control" role="group" aria-label="Modo de ejecucion">
              <button
                type="button"
                className={threadMode === THREAD_MODES.single ? "active" : ""}
                onClick={() => setThreadMode(THREAD_MODES.single)}
              >
                Un valor
              </button>
              <button
                type="button"
                className={threadMode === THREAD_MODES.compare ? "active" : ""}
                onClick={() => setThreadMode(THREAD_MODES.compare)}
              >
                Comparar 6/12/18
              </button>
            </div>
            <div
              className={`segmented-control ${threadMode === THREAD_MODES.compare ? "disabled" : ""}`}
              role="group"
              aria-label="Cantidad de hilos"
            >
              {THREAD_OPTIONS.map((option) => (
                <button
                  key={option}
                  type="button"
                  className={threads === option ? "active" : ""}
                  disabled={threadMode === THREAD_MODES.compare}
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
              <button className="mini-button" type="button" onClick={toggleAllTransforms}>
                {allTransformsSelected ? "Quitar todas" : "Todas"}
              </button>
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
                  {activeThread ? `Procesando ${activeThread}` : "Procesando"}
                </>
              ) : (
                <>
                <Play size={18} aria-hidden="true" />
                  {threadMode === THREAD_MODES.compare ? "Comparar" : "Procesar"}
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

          {lastRun?.outputImages?.length > 0 && (
            <section className="section-block">
              <div className="section-heading">
                <Image size={20} aria-hidden="true" />
                <h2>Imagenes producidas</h2>
                <span className="counter">{lastRun.outputImages.length}</span>
                <button
                  className="mini-button download-action"
                  type="button"
                  onClick={downloadResults}
                  disabled={downloadStatus === "running"}
                >
                  {downloadStatus === "running" ? (
                    <Loader2 className="spin" size={16} aria-hidden="true" />
                  ) : (
                    <Download size={16} aria-hidden="true" />
                  )}
                  ZIP
                </button>
              </div>
              <div className="output-gallery">
                {lastRun.outputImages.map((image) => (
                  <figure className="output-image-card" key={`${image.filename}-${image.url}`}>
                    <img src={image.url} alt={`${image.label} imagen ${image.image_index}`} />
                    <figcaption>
                      <strong>{image.label}</strong>
                      <span>Imagen {image.image_index}</span>
                    </figcaption>
                  </figure>
                ))}
              </div>
            </section>
          )}

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

  const items = run.comparison
    ? [
        { label: "Mejor C", value: `${run.executionTime.toFixed(6)} s` },
        { label: "Total C", value: `${run.totalExecutionTime.toFixed(6)} s` },
        { label: "Imagenes", value: run.images },
        { label: "Mejor hilos", value: run.bestThread },
      ]
    : [
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
      {run.comparison && (
        <div className="comparison-table">
          {run.comparison.map((item) => (
            <div key={item.id}>
              <strong>{item.threads} hilos</strong>
              <span>{item.executionTime.toFixed(6)} s</span>
            </div>
          ))}
        </div>
      )}
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

function createComparisonRun(runs) {
  const bestRun = runs.reduce(
    (best, run) => (run.executionTime < best.executionTime ? run : best),
    runs[0],
  );
  const lastRun = runs[runs.length - 1];

  return {
    ...lastRun,
    id: `comparison-${Date.now()}`,
    threads: THREAD_OPTIONS.join(", "),
    executionTime: bestRun.executionTime,
    totalExecutionTime: runs.reduce((sum, run) => sum + run.executionTime, 0),
    clientSeconds: runs.reduce((sum, run) => sum + run.clientSeconds, 0),
    bestThread: bestRun.threads,
    comparison: runs,
    outputImages: lastRun.outputImages,
  };
}

function resolveOutputUrl(url, cacheKey) {
  const absoluteUrl = resolveBackendUrl(url);
  const separator = absoluteUrl.includes("?") ? "&" : "?";
  return `${absoluteUrl}${separator}v=${cacheKey}`;
}

function resolveBackendUrl(path) {
  const base = API_URL.startsWith("http") ? API_URL : window.location.origin;
  return new URL(path, base).href;
}

export default App;
