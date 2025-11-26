import { useMetrics } from "../hooks/useMetrics";
import { Activity } from "lucide-react";

export function IoTDashboard() {
  const metrics = useMetrics();   // <-- get live metrics from Python

  return (
    <div className="space-y-6">

      {/* Statistics Cards */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">

        {/* Download Speed */}
        <div className="bg-gradient-to-br from-[#BF915A] to-[#A24E35] rounded-3xl shadow-sm p-6 text-white">
          <div className="text-sm text-white/80 mb-2">Average Download Speed</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{metrics.avg_download.toFixed(1)}</span>
            <span className="text-lg text-white/80">MBps</span>
          </div>
        </div>

        {/* Upload Speed */}
        <div className="bg-gradient-to-br from-[#80949D] to-[#09181A] rounded-3xl shadow-sm p-6 text-white">
          <div className="text-sm text-white/80 mb-2">Average Upload Speed</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{metrics.avg_upload.toFixed(1)}</span>
            <span className="text-lg text-white/80">MBps</span>
          </div>
        </div>

        {/* Current Time */}
        <div className="bg-gradient-to-br from-[#C6C2A4] to-[#BF915A] rounded-3xl shadow-sm p-6 text-[#09181A]">
          <div className="text-sm text-[#09181A]/70 mb-2">Current Time</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{metrics.current_time}</span>
            <span className="text-lg text-[#09181A]/70">UTC-6</span>
          </div>
        </div>

      </div>

      {/* Plot */}
      <div className="bg-white rounded-3xl shadow-sm p-8 transition-all hover:shadow-md">
        <div className="mb-6">
          <h2 className="text-gray-900 mb-1">Real-Time Data Visualization</h2>
          <p className="text-gray-500">Live throughput mapping</p>
        </div>

        <div className="w-full h-[600px] rounded-3xl overflow-hidden">
          <iframe
            src="http://localhost:5000/plot"
            className="w-full h-full border-0"
          />
        </div>

      </div>
    </div>
  );
}
