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
      fetch("http://172.20.10.10:8080/metrics") // Franco
      // fetch("http://10.50.77.144:8080/metrics")
        .then(res => res.json())
        .then(setMetrics)
        .catch(() => {});
    }, 1000);

    return () => clearInterval(interval);
  }, []);

  return metrics;
}
