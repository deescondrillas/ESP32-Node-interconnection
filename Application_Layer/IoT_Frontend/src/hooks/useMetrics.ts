import { useEffect, useState } from "react";

export function useMetrics() {
  const [metrics, setMetrics] = useState({
    avg_download: 0,
    avg_upload: 0,
    num_queries: 0,
    current_time: ""
  });

  useEffect(() => {
    const interval = setInterval(() => {
      fetch("http://localhost:5000/metrics")
        .then(res => res.json())
        .then(setMetrics)
        .catch(() => {});
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  return metrics;
}
