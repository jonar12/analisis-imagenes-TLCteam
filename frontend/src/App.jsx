import React, { useEffect, useMemo, useRef, useState } from "react";
import {
  AlertCircle,
  BarChart3,
  Check,
  Clock3,
  Cpu,
  Download,
  FolderOpen,
  Gauge,
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
const MAX_FILES = 250;
const MIN_BMP_SIDE = 2000;
const KERNEL_MIN = 55;
const KERNEL_MAX = 155;
const DEFAULT_PIXELS_PER_SECOND = 60_000_000;
const THREAD_OPTIONS = [6, 12, 18];
const THREAD_MODES = {
  single: "single",
  compare: "compare",
};

const TRANSFORMATIONS = [
  { key: "grey_h", label: "Escala de grises horizontal", accent: "teal" },
  { key: "color_v", label: "Color vertical", accent: "blue" },
  { key: "color_h", label: "Color horizontal", accent: "coral" },
  { key: "blur_color", label: "Desenfoque a color", accent: "amber" },
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
  const [kernelColor, setKernelColor] = useState(KERNEL_MIN);
  const [status, setStatus] = useState("idle");
  const [activeThread, setActiveThread] = useState(null);
  const [downloadStatus, setDownloadStatus] = useState("idle");
  const [error, setError] = useState("");
  const [lastRun, setLastRun] = useState(null);
  const [history, setHistory] = useState([]);
  const [progressState, setProgressState] = useState(null);
  const [progressTick, setProgressTick] = useState(0);

  const selectedTransforms = useMemo(
    () => TRANSFORMATIONS.filter((item) => transforms[item.key]),
    [transforms],
  );
  const allTransformsSelected = selectedTransforms.length === TRANSFORMATIONS.length;

  const totalSize = useMemo(
    () => files.reduce((sum, entry) => sum + entry.file.size, 0),
    [files],
  );

  const imagePixels = useMemo(
    () => files.reduce((sum, entry) => sum + entry.pixels, 0),
    [files],
  );

  const workload = useMemo(
    () => createWorkloadSummary(files, selectedTransforms, threadMode, history),
    [files, selectedTransforms, threadMode, history],
  );

  useEffect(() => {
    if (status !== "running") return undefined;

    const interval = window.setInterval(() => {
      setProgressTick((current) => current + 1);
    }, 500);

    return () => window.clearInterval(interval);
  }, [status]);

  async function addFiles(fileList) {
    const incoming = Array.from(fileList ?? []);
    if (incoming.length === 0) return;

    const parsedFiles = await Promise.all(incoming.map(createBmpEntry));
    const validEntries = parsedFiles
      .filter((item) => item.entry)
      .map((item) => item.entry);
    const validationErrors = parsedFiles
      .filter((item) => item.error)
      .map((item) => item.error);

    const known = new Set(files.map((entry) => entry.id));
    const unique = validEntries.filter((entry) => !known.has(entry.id));
    const availableSlots = Math.max(0, MAX_FILES - files.length);
    const accepted = unique.slice(0, availableSlots);
    const messages = [...validationErrors];

    if (unique.length < validEntries.length) {
      messages.push("Se omitieron archivos duplicados");
    }

    if (unique.length > availableSlots) {
      messages.push(`Maximo ${MAX_FILES} imagenes BMP por corrida`);
    }

    setFiles((current) => [...current, ...accepted]);
    setError(messages[0] ?? "");

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

  function normalizeKernel(value) {
    const parsed = Math.trunc(Number(value));
    if (!Number.isFinite(parsed)) return KERNEL_MIN;

    const clamped = Math.min(KERNEL_MAX, Math.max(KERNEL_MIN, parsed));
    if (clamped % 2 !== 0) return clamped;
    return clamped >= KERNEL_MAX ? KERNEL_MAX : clamped + 1;
  }

  function createOptions(threadCount) {
    const flags = TRANSFORMATIONS.reduce(
      (acc, item) => ({ ...acc, [item.key]: Boolean(transforms[item.key]) }),
      {},
    );

    return {
      ...flags,
      threads: threadCount,
      kernel_color: normalizeKernel(kernelColor),
    };
  }

  async function runCompiler(threadCount, selectedLabels) {
    const options = createOptions(threadCount);
    const body = new FormData();
    files.forEach((entry) => body.append("images", entry.file));
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
    const totalPixels = Number(data?.metrics?.total_pixels ?? 0);
    const pixelsPerSecond = Number(data?.metrics?.pixels_per_second ?? 0);
    const runId = `${Date.now()}-${threadCount}`;

    return {
      id: runId,
      images: files.length,
      threads: threadCount,
      transformations: selectedLabels,
      kernelColor: options.kernel_color,
      usesBlur: Boolean(options.blur_color),
      executionTime: Number.isFinite(executionTime) ? executionTime : 0,
      clientSeconds,
      totalPixels: Number.isFinite(totalPixels) ? totalPixels : 0,
      pixelsPerSecond: Number.isFinite(pixelsPerSecond) ? pixelsPerSecond : 0,
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
      setError("Agrega al menos una imagen BMP valida");
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
    setProgressState({
      ...workload,
      startedAt: performance.now(),
      completedRuns: 0,
      totalRuns: threadList.length,
    });

    try {
      const runs = [];
      for (const threadCount of threadList) {
        setActiveThread(threadCount);
        const run = await runCompiler(threadCount, selectedLabels);
        runs.push(run);
        setProgressState((current) => (
          current
            ? { ...current, completedRuns: runs.length }
            : current
        ));
      }

      const nextRun = threadMode === THREAD_MODES.compare ? createComparisonRun(runs) : runs[0];
      setLastRun(nextRun);
      setHistory((current) => [...current, ...runs].slice(-12));
      setStatus("done");
      setProgressState((current) => (
        current
          ? {
              ...current,
              completedRuns: current.totalRuns,
              finishedAt: performance.now(),
            }
          : current
      ));
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
    setKernelColor(KERNEL_MIN);
    setError("");
    setActiveThread(null);
    setDownloadStatus("idle");
    setLastRun(null);
    setProgressState(null);
    setProgressTick(0);
    setStatus("idle");
  }

  return (
    <div className="app-shell">
      <header className="topbar">
        <div className="brand-lockup">
          <img className="tec-logo" src={tecLogo} alt="Tecnologico de Monterrey" />
          <div>
            <h1>TLC Image Lab</h1>
            <p>Procesamiento BMP hibrido MPI + OpenMP en x86</p>
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
              <small>
                {files.length > 0
                  ? `${formatBytes(totalSize)} / ${formatPixels(imagePixels)}`
                  : `BMP con lado mayor a ${MIN_BMP_SIDE}px`}
              </small>
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
                {files.map((entry, index) => (
                  <div className="file-card" key={entry.id}>
                    <Image size={18} aria-hidden="true" />
                    <div className="file-meta">
                      <strong>{entry.file.name}</strong>
                      <span>
                        {formatBytes(entry.file.size)} / {entry.width}x{entry.height}px
                      </span>
                    </div>
                    <button
                      className="icon-button"
                      type="button"
                      aria-label={`Quitar ${entry.file.name}`}
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
              <h2>Hilos OpenMP</h2>
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
                  className={threads === option || threadMode === THREAD_MODES.compare ? "active" : ""}
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
            <label className={!transforms.blur_color ? "disabled" : ""}>
              <span>Kernel color</span>
              <input
                type="number"
                min={KERNEL_MIN}
                max={KERNEL_MAX}
                step="2"
                value={kernelColor}
                disabled={!transforms.blur_color}
                onChange={(event) => setKernelColor(event.target.value)}
                onBlur={() => setKernelColor(normalizeKernel(kernelColor))}
              />
            </label>
            <div className="kernel-limit">
              <span>Rango</span>
              <strong>{KERNEL_MIN}-{KERNEL_MAX}</strong>
            </div>
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
            <button className="primary-button" type="submit" disabled={status === "running"}>
              {status === "running" ? (
                <>
                  <Loader2 className="spin" size={18} aria-hidden="true" />
                  Procesando
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
              <Gauge size={20} aria-hidden="true" />
              <h2>Avance de carga</h2>
            </div>
            <WorkloadProgress
              activeThread={activeThread}
              progress={progressState}
              status={status}
              tick={progressTick}
              workload={workload}
            />
          </section>

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
        </aside>
      </main>
    </div>
  );
}

function WorkloadProgress({ activeThread, progress, status, tick, workload }) {
  void tick;

  const hasWorkload = workload.workloadPixels > 0;
  const isRunning = status === "running" && progress?.startedAt;
  const elapsedSeconds = isRunning
    ? (performance.now() - progress.startedAt) / 1000
    : progress?.finishedAt && progress?.startedAt
      ? (progress.finishedAt - progress.startedAt) / 1000
      : 0;
  const estimatedSeconds = progress?.estimatedSeconds ?? workload.estimatedSeconds;
  const totalRuns = progress?.totalRuns ?? workload.runCount;
  const completedRuns = progress?.completedRuns ?? 0;
  const runRatio = totalRuns > 0 ? completedRuns / totalRuns : 0;
  const timeRatio = estimatedSeconds > 0 ? elapsedSeconds / estimatedSeconds : 0;
  const progressRatio = !hasWorkload
    ? 0
    : isRunning
      ? Math.min(0.97, Math.max(runRatio, timeRatio))
      : status === "done" && progress
        ? 1
        : 0;
  const remainingSeconds = isRunning
    ? Math.max(0, estimatedSeconds - elapsedSeconds)
    : estimatedSeconds;
  const percent = Math.round(progressRatio * 100);
  const stage = activeThread
    ? `${activeThread} hilos`
    : `${workload.runCount} ${workload.runCount === 1 ? "corrida" : "corridas"}`;

  return (
    <div className="eta-panel">
      <div className="eta-ring" style={{ "--progress-deg": `${progressRatio * 360}deg` }}>
        <strong>{percent}%</strong>
        <span>{isRunning ? "Activo" : "Estimado"}</span>
      </div>
      <div className="eta-details">
        <div>
          <span>Restante</span>
          <strong>{hasWorkload ? formatDuration(remainingSeconds) : "--"}</strong>
        </div>
        <div>
          <span>Carga</span>
          <strong>{formatPixels(workload.workloadPixels)}</strong>
        </div>
        <div>
          <span>Etapa</span>
          <strong>{hasWorkload ? stage : "--"}</strong>
        </div>
      </div>
      <div className="eta-track" aria-label="Avance estimado">
        <span style={{ width: `${percent}%` }} />
      </div>
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
        { label: "Mejor tiempo", value: `${run.executionTime.toFixed(6)} s` },
        { label: "Total C", value: `${run.totalExecutionTime.toFixed(6)} s` },
        { label: "Imagenes", value: run.images },
        { label: "Mejor hilos", value: run.bestThread },
        { label: "Rendimiento", value: formatPixelsPerSecond(run.pixelsPerSecond) },
        { label: "Kernel", value: run.usesBlur ? run.kernelColor : "N/A" },
      ]
    : [
        { label: "Tiempo MPI", value: `${run.executionTime.toFixed(6)} s` },
        { label: "Request", value: `${run.clientSeconds.toFixed(3)} s` },
        { label: "Imagenes", value: run.images },
        { label: "Hilos", value: run.threads },
        { label: "Rendimiento", value: formatPixelsPerSecond(run.pixelsPerSecond) },
        { label: "Kernel", value: run.usesBlur ? run.kernelColor : "N/A" },
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

async function createBmpEntry(file) {
  const id = createFileId(file);

  if (!file.name.toLowerCase().endsWith(".bmp")) {
    return { error: `${file.name}: solo .bmp` };
  }

  try {
    const metadata = await readBmpMetadata(file);
    const longSide = Math.max(metadata.width, metadata.height);

    if (longSide <= MIN_BMP_SIDE) {
      return { error: `${file.name}: requiere mas de ${MIN_BMP_SIDE}px` };
    }

    return {
      entry: {
        id,
        file,
        ...metadata,
        pixels: metadata.width * metadata.height,
      },
    };
  } catch {
    return { error: `${file.name}: BMP invalido` };
  }
}

async function readBmpMetadata(file) {
  const header = await file.slice(0, 26).arrayBuffer();
  if (header.byteLength < 26) {
    throw new Error("BMP header too small");
  }

  const view = new DataView(header);
  const isBmp = view.getUint8(0) === 0x42 && view.getUint8(1) === 0x4d;
  if (!isBmp) {
    throw new Error("Invalid BMP signature");
  }

  const width = Math.abs(view.getInt32(18, true));
  const height = Math.abs(view.getInt32(22, true));
  if (!width || !height) {
    throw new Error("Invalid BMP dimensions");
  }

  return { width, height };
}

function createWorkloadSummary(files, selectedTransforms, threadMode, history) {
  const imagePixels = files.reduce((sum, entry) => sum + entry.pixels, 0);
  const transformCount = selectedTransforms.length;
  const runCount = threadMode === THREAD_MODES.compare ? THREAD_OPTIONS.length : 1;
  const workloadPixels = imagePixels * transformCount * runCount;
  const throughput = estimateThroughput(history);

  return {
    estimatedSeconds: workloadPixels > 0 ? workloadPixels / throughput.value : 0,
    imagePixels,
    runCount,
    throughput,
    transformCount,
    workloadPixels,
  };
}

function estimateThroughput(history) {
  const recentRuns = history
    .filter((run) => run.totalPixels > 0 && run.executionTime > 0)
    .slice(-5);

  if (recentRuns.length === 0) {
    return { measured: false, value: DEFAULT_PIXELS_PER_SECOND };
  }

  const totalPixels = recentRuns.reduce((sum, run) => sum + run.totalPixels, 0);
  const totalSeconds = recentRuns.reduce((sum, run) => sum + run.executionTime, 0);
  return {
    measured: true,
    value: totalSeconds > 0 ? totalPixels / totalSeconds : DEFAULT_PIXELS_PER_SECOND,
  };
}

function createFileId(file) {
  return `${file.name}-${file.size}-${file.lastModified}`;
}

function formatBytes(bytes) {
  if (!bytes) return "0 KB";
  const units = ["B", "KB", "MB", "GB"];
  const index = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const value = bytes / 1024 ** index;
  return `${value.toFixed(index === 0 ? 0 : 1)} ${units[index]}`;
}

function formatPixels(pixels) {
  if (!pixels) return "0 px";
  const units = ["px", "Kpx", "Mpx", "Gpx"];
  const index = Math.min(Math.floor(Math.log(pixels) / Math.log(1000)), units.length - 1);
  const value = pixels / 1000 ** index;
  return `${value.toFixed(index === 0 ? 0 : 1)} ${units[index]}`;
}

function formatPixelsPerSecond(pixelsPerSecond) {
  if (!pixelsPerSecond) return "0 px/s";
  return `${formatPixels(pixelsPerSecond)}/s`;
}

function formatDuration(seconds) {
  if (!Number.isFinite(seconds) || seconds <= 0) return "0 s";
  if (seconds < 1) return "<1 s";

  const wholeSeconds = Math.ceil(seconds);
  const hours = Math.floor(wholeSeconds / 3600);
  const minutes = Math.floor((wholeSeconds % 3600) / 60);
  const restSeconds = wholeSeconds % 60;

  if (hours > 0) return `${hours} h ${minutes} min`;
  if (minutes > 0) return `${minutes} min ${restSeconds} s`;
  return `${restSeconds} s`;
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
    totalPixels: runs.reduce((sum, run) => sum + run.totalPixels, 0),
    pixelsPerSecond: bestRun.pixelsPerSecond,
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
