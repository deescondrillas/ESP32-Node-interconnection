import { useEffect, useState } from "react";
import { ThreeDScatterPlot } from "./ThreeDScatterPlot";
import { Activity } from "lucide-react";

export interface IoTDataPoint {
  id: number;
  temperature: number;
  humidity: number;
  pressure: number;
  deviceId: string;
}

// Mock API call - simulates fetching data from an IoT analytics API
const fetchIoTData = async (): Promise<IoTDataPoint[]> => {
  // Simulate API delay
  await new Promise(resolve => setTimeout(resolve, 1000));
  
  // Generate mock data points
  const data: IoTDataPoint[] = [];
  
  for (let i = 0; i < 50; i++) {
    data.push({
      id: i,
      temperature: 15 + Math.random() * 20,
      humidity: 30 + Math.random() * 50,
      pressure: 990 + Math.random() * 40,
      deviceId: `Device-${Math.floor(i / 5) + 1}`
    });
  }
  
  return data;
};

export function IoTDashboard() {
  const [data, setData] = useState<IoTDataPoint[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    fetchIoTData().then(result => {
      setData(result);
      setLoading(false);
    });
  }, []);

  if (loading) {
    return (
      <div className="bg-white rounded-3xl shadow-sm p-12 flex items-center justify-center">
        <div className="flex items-center gap-3 text-gray-500">
          <Activity className="w-5 h-5 animate-pulse" />
          <span>Loading sensor data...</span>
        </div>
      </div>
    );
  }

  // Calculate statistics
  const avgTemp = (data.reduce((sum, d) => sum + d.temperature, 0) / data.length).toFixed(1);
  const avgHumidity = (data.reduce((sum, d) => sum + d.humidity, 0) / data.length).toFixed(1);
  const avgPressure = (data.reduce((sum, d) => sum + d.pressure, 0) / data.length).toFixed(1);

  return (
    <div className="space-y-6">
      {/* Statistics Cards */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div className="bg-gradient-to-br from-[#BF915A] to-[#A24E35] rounded-3xl shadow-sm p-6 text-white">
          <div className="text-sm text-white/80 mb-2">Average Download Speed</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{avgTemp}</span>
            <span className="text-lg text-white/80">MBps</span>
          </div>
        </div>
        
        <div className="bg-gradient-to-br from-[#80949D] to-[#09181A] rounded-3xl shadow-sm p-6 text-white">
          <div className="text-sm text-white/80 mb-2">Average Upload Speed</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{avgHumidity}</span>
            <span className="text-lg text-white/80">MBps</span>
          </div>
        </div>
        
        <div className="bg-gradient-to-br from-[#C6C2A4] to-[#BF915A] rounded-3xl shadow-sm p-6 text-[#09181A]">
          <div className="text-sm text-[#09181A]/70 mb-2">Current Time</div>
          <div className="flex items-baseline gap-2">
            <span className="text-3xl">{avgPressure}</span>
            <span className="text-lg text-[#09181A]/70">UTC-6</span>
          </div>
        </div>
      </div>

      {/* 3D Visualization */}
      <div className="bg-white rounded-3xl shadow-sm p-8 transition-all hover:shadow-md">
        <div className="mb-6">
          <h2 className="text-gray-900 mb-1">3D Data Visualization</h2>
          <p className="text-gray-500">Latitude, Longitude, and Throughput</p>
        </div>
        
        <ThreeDScatterPlot data={data} />
        
        <div className="mt-6 flex items-center justify-center gap-8">
          <div className="flex items-center gap-2">
            <div className="w-4 h-4 rounded-full bg-[#BF915A]"></div>
            <span className="text-sm text-gray-600">X: Latitude (m)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-4 h-4 rounded-full bg-[#80949D]"></div>
            <span className="text-sm text-gray-600">Y: Longitude (m)</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="w-4 h-4 rounded-full bg-[#A24E35]"></div>
            <span className="text-sm text-gray-600">Z: Throughput (MBps)</span>
          </div>
        </div>
      </div>
    </div>
  );
}