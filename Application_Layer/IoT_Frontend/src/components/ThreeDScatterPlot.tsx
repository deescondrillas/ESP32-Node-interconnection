import { useRef, useEffect, useState } from "react";
import { Canvas, useFrame } from "@react-three/fiber";
import { OrbitControls, PerspectiveCamera } from "@react-three/drei";
import { IoTDataPoint } from "./IoTDashboard";

interface DataPointProps {
  position: [number, number, number];
  color: string;
  temperature: number;
}

function DataSphere({ position, color, temperature }: DataPointProps) {
  const meshRef = useRef<any>(null);
  const [hovered, setHovered] = useState(false);

  useFrame((state) => {
    if (meshRef.current) {
      // Subtle floating animation
      meshRef.current.position.y = position[1] + Math.sin(state.clock.elapsedTime + position[0]) * 0.05;
    }
  });

  return (
    <mesh
      ref={meshRef}
      position={position}
      onPointerOver={() => setHovered(true)}
      onPointerOut={() => setHovered(false)}
      scale={hovered ? 1.3 : 1}
    >
      <sphereGeometry args={[0.15, 16, 16]} />
      <meshStandardMaterial
        color={color}
        emissive={color}
        emissiveIntensity={hovered ? 0.5 : 0.2}
        metalness={0.3}
        roughness={0.4}
      />
    </mesh>
  );
}

function AxesHelper() {
  return (
    <group>
      {/* X Axis - Temperature */}
      <line>
        <bufferGeometry>
          <bufferAttribute
            attach="attributes-position"
            count={2}
            array={new Float32Array([0, 0, 0, 5, 0, 0])}
            itemSize={3}
          />
        </bufferGeometry>
        <lineBasicMaterial color="#BF915A" linewidth={2} />
      </line>
      
      {/* Y Axis - Humidity */}
      <line>
        <bufferGeometry>
          <bufferAttribute
            attach="attributes-position"
            count={2}
            array={new Float32Array([0, 0, 0, 0, 5, 0])}
            itemSize={3}
          />
        </bufferGeometry>
        <lineBasicMaterial color="#80949D" linewidth={2} />
      </line>
      
      {/* Z Axis - Pressure */}
      <line>
        <bufferGeometry>
          <bufferAttribute
            attach="attributes-position"
            count={2}
            array={new Float32Array([0, 0, 0, 0, 0, 5])}
            itemSize={3}
          />
        </bufferGeometry>
        <lineBasicMaterial color="#A24E35" linewidth={2} />
      </line>
    </group>
  );
}

interface ThreeDScatterPlotProps {
  data: IoTDataPoint[];
}

export function ThreeDScatterPlot({ data }: ThreeDScatterPlotProps) {
  // Normalize data to fit in a reasonable 3D space
  const normalizeData = (data: IoTDataPoint[]) => {
    const temps = data.map(d => d.temperature);
    const humidities = data.map(d => d.humidity);
    const pressures = data.map(d => d.pressure);
    
    const tempMin = Math.min(...temps);
    const tempMax = Math.max(...temps);
    const humidityMin = Math.min(...humidities);
    const humidityMax = Math.max(...humidities);
    const pressureMin = Math.min(...pressures);
    const pressureMax = Math.max(...pressures);
    
    // Color palette inspired by Protoboard and ESP32
    const colors = ['#BF915A', '#80949D', '#A24E35', '#C6C2A4', '#09181A'];
    
    return data.map(point => {
      const x = ((point.temperature - tempMin) / (tempMax - tempMin)) * 4 - 2;
      const y = ((point.humidity - humidityMin) / (humidityMax - humidityMin)) * 4 - 2;
      const z = ((point.pressure - pressureMin) / (pressureMax - pressureMin)) * 4 - 2;
      
      // Color based on temperature value using custom palette
      const tempNorm = (point.temperature - tempMin) / (tempMax - tempMin);
      const colorIndex = Math.floor(tempNorm * (colors.length - 1));
      const color = colors[colorIndex];
      
      return {
        position: [x, y, z] as [number, number, number],
        color,
        temperature: point.temperature
      };
    });
  };

  const normalizedData = normalizeData(data);

  return (
    <div className="w-full h-[600px] bg-gradient-to-br from-gray-50 to-gray-100 rounded-2xl overflow-hidden">
      <Canvas>
        <PerspectiveCamera makeDefault position={[8, 6, 8]} />
        <OrbitControls 
          enablePan={true}
          enableZoom={true}
          enableRotate={true}
          autoRotate={true}
          autoRotateSpeed={0.5}
        />
        
        {/* Lighting */}
        <ambientLight intensity={0.5} />
        <pointLight position={[10, 10, 10]} intensity={1} />
        <pointLight position={[-10, -10, -10]} intensity={0.5} />
        <directionalLight position={[5, 5, 5]} intensity={0.5} />
        
        {/* Axes */}
        <AxesHelper />
        
        {/* Grid */}
        <gridHelper args={[10, 10, "#e5e7eb", "#f3f4f6"]} position={[0, -2.5, 0]} />
        
        {/* Data Points */}
        {normalizedData.map((point, index) => (
          <DataSphere
            key={index}
            position={point.position}
            color={point.color}
            temperature={point.temperature}
          />
        ))}
      </Canvas>
    </div>
  );
}