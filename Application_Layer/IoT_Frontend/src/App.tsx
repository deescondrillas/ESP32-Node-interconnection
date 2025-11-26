import { IoTDashboard } from "./components/IoTDashboard";
import { Activity, Wifi } from "lucide-react";
import { useMetrics } from "./hooks/useMetrics";

export default function App() {
  const metrics = useMetrics();

  return (
    <div className="min-h-screen bg-gray-50">

      {/* Header */}
      <header className="relative bg-gradient-to-r from-[#09181A] via-[#80949D] to-[#BF915A] text-white overflow-hidden">
        <div className="absolute inset-0 bg-black/10"></div>

        <div className="relative max-w-7xl mx-auto px-6 py-12">
          <div className="flex items-center gap-3 mb-3">
            <div className="p-3 bg-white/20 backdrop-blur-sm rounded-2xl">
              <Activity className="w-7 h-7" />
            </div>
            <h1 className="text-white">ESP32 Node Interconnection</h1>
          </div>

          <p className="text-white/90 max-w-2xl">
            Real-time monitoring and visualization of sensor data
          </p>

          <div className="flex items-center gap-2 mt-6">
            <Wifi className="w-4 h-4 text-[#C6C2A4]" />
            <span className="text-sm text-white/80">
              Live Connection • {metrics.num_queries} MQTT packages successfully received
            </span>
          </div>
        </div>

        {/* Decorative gradients */}
        <div className="absolute top-0 right-0 w-96 h-96 bg-[#BF915A]/20 rounded-full blur-3xl -translate-y-1/2 translate-x-1/2"></div>
        <div className="absolute bottom-0 left-1/4 w-64 h-64 bg-[#A24E35]/20 rounded-full blur-3xl translate-y-1/2"></div>
      </header>

      {/* Main */}
      <div className="max-w-7xl mx-auto px-6 py-12">
        <IoTDashboard />
      </div>

    </div>
  );
}
