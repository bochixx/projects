import 'package:flutter/material.dart';
import 'dart:async';
import 'dart:math';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

void main() {
  runApp(const TennisTrainingApp());
}

class TennisTrainingApp extends StatelessWidget {
  const TennisTrainingApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Tennis Training',
      theme: ThemeData(primarySwatch: Colors.blue),
      home: const BLEDeviceScreen(),
    );
  }
}

class BLEDeviceScreen extends StatefulWidget {
  const BLEDeviceScreen({super.key});

  @override
  State<BLEDeviceScreen> createState() => _BLEDeviceScreenState();
}

class _BLEDeviceScreenState extends State<BLEDeviceScreen> {
  /*final FlutterBluePlus flutterBlue = FlutterBluePlus.instance;*/
  final List<ScanResult> devices = [];



  @override
  void initState() {
    super.initState();
    _enableBluetoothAndScan();
  }

  //void _enableBluetoothAndScan() async {
    //_startScan();
  //}
  void _enableBluetoothAndScan() async {
    if (!await FlutterBluePlus.adapterState.first.then((state) => state == BluetoothAdapterState.on)) {
      await FlutterBluePlus.turnOn();
    }
    _startScan();
  }


  void _startScan() {
    devices.clear();
    FlutterBluePlus.startScan(timeout: const Duration(seconds: 4));
    FlutterBluePlus.scanResults.listen((results) {
      for (var result in results) {
        if (!devices.any((d) => d.device.id == result.device.id)) {
          setState(() => devices.add(result));
        }
      }
    });
  }

  void _connectToDevice(BluetoothDevice device) async {
    await FlutterBluePlus.stopScan();
    await device.connect();
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text("Connected to ${device.name}")),
    );
    Navigator.pushReplacement(
      context,
      MaterialPageRoute(
        builder: (context) => TennisTrainingScreen(device: device),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Select BLE Device")),
      body: Column(
        children: [
          ElevatedButton(
            onPressed: _startScan,
            child: const Text("Refresh Device List"),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: devices.length,
              itemBuilder: (context, index) {
                final device = devices[index].device;
                return ListTile(
                  title: Text(device.name.isNotEmpty ? device.name : "Unknown"),
                  subtitle: Text(device.id.id),
                  onTap: () => _connectToDevice(device),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}

class TennisTrainingScreen extends StatefulWidget {
  final BluetoothDevice device;
  const TennisTrainingScreen({super.key, required this.device});

  @override
  State<TennisTrainingScreen> createState() => _TennisTrainingScreenState();
}

class _TennisTrainingScreenState extends State<TennisTrainingScreen> {
  int backhandCount = 0;
  int forehandCount = 0;
  BluetoothCharacteristic? resultChar;
  BluetoothCharacteristic? commandChar;

  @override
  void initState() {
    super.initState();
    _setupConnection();
  }

  void _setupConnection() async {
    List<BluetoothService> services = await widget.device.discoverServices();
    for (var service in services) {
      for (var char in service.characteristics) {
        if (char.uuid.toString().contains("2a57")) {
          resultChar = char;
          await char.setNotifyValue(true);
          char.onValueReceived.listen((value) {
            final str = String.fromCharCodes(value);
            if (str == "Forehand") {
              setState(() => forehandCount++);
            } else if (str == "Backhand") {
              setState(() => backhandCount++);
            }
          });
        } else if (char.uuid.toString().contains("2a58")) {
          commandChar = char;
        }
      }
    }
  }

  void _sendCommand(String cmd) async {
    if (commandChar != null) {
      await commandChar!.write(cmd.codeUnits);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: Column(
        children: [
          const SizedBox(height: 50),
          const Text(
            'Tennis Training',
            style: TextStyle(fontSize: 32, fontWeight: FontWeight.bold, color: Colors.black),
          ),
          const SizedBox(height: 80),
          Row(
            children: [
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(10),
                  child: Image.asset('assets/1.png', fit: BoxFit.contain),
                ),
              ),
              Expanded(
                child: Padding(
                  padding: const EdgeInsets.all(10),
                  child: Image.asset('assets/2.png', fit: BoxFit.contain),
                ),
              ),
            ],
          ),
          const SizedBox(height: 80),
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Column(
                children: [
                  Stack(
                    alignment: Alignment.center,
                    children: [
                      Image.asset('assets/3.png', width: 80, height: 80),
                      Text('$forehandCount', style: const TextStyle(fontSize: 28, fontWeight: FontWeight.bold)),
                    ],
                  ),
                  const SizedBox(height: 10),
                  const Text('Forehand', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                ],
              ),
              const SizedBox(width: 100),
              Column(
                children: [
                  Stack(
                    alignment: Alignment.center,
                    children: [
                      Image.asset('assets/4.png', width: 80, height: 80),
                      Text('$backhandCount', style: const TextStyle(fontSize: 28, fontWeight: FontWeight.bold)),
                    ],
                  ),
                  const SizedBox(height: 10),
                  const Text('Backhand', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                ],
              ),
            ],
          ),
          const SizedBox(height: 40),
          ElevatedButton(
            onPressed: () {
              _sendCommand("TRAINING_MODE");
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => TrainingPage(sendCommand: _sendCommand, resultChar: resultChar!),
                ),
              ).then((_) => _sendCommand("EXIT_TRAINING"));
            },
            style: ElevatedButton.styleFrom(
              padding: const EdgeInsets.symmetric(horizontal: 50, vertical: 20),
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(30)),
              backgroundColor: Colors.orange,
              elevation: 10,
            ),
            child: const Text('Training Module', style: TextStyle(fontSize: 22, fontWeight: FontWeight.bold)),
          ),
        ],
      ),
    );
  }
}

class TrainingPage extends StatefulWidget {
  final void Function(String) sendCommand;
  final BluetoothCharacteristic resultChar;
  const TrainingPage({super.key, required this.sendCommand, required this.resultChar});

  @override
  State<TrainingPage> createState() => _TrainingPageState();
}

class _TrainingPageState extends State<TrainingPage> {
  String instruction = '';
  String result = '';
  bool started = false;
  DateTime lastTime = DateTime.now().subtract(const Duration(seconds: 5));
  final random = Random();
  late StreamSubscription<List<int>> resultSub;

  @override
  void initState() {
    super.initState();
    resultSub = widget.resultChar.onValueReceived.listen((value) {
      setState(() => result = String.fromCharCodes(value));
    });
  }

  @override
  void dispose() {
    resultSub.cancel();
    super.dispose();
  }

  void generateInstruction() {
    if (DateTime.now().difference(lastTime).inSeconds < 3) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text("You have to wait 3 seconds between each training action."),
          duration: Duration(seconds: 2),
        ),
      );
      return;
    }
    setState(() {
      instruction = random.nextBool() ? "Do Forehand" : "Do Backhand";
      result = "";
      lastTime = DateTime.now();
    });
    widget.sendCommand(instruction.split(" ")[1]);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Training Mode'),
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () => Navigator.pop(context),
        ),
      ),
      body: Center(
        child: Column(
          children: [
            const SizedBox(height: 20),
            const Padding(
              padding: EdgeInsets.symmetric(horizontal: 16.0),
              child: Text(
                "Welcome to the tennis training part. Click the button to start and follow the instruction showed on the screen. You will be notified whether you have done it correct or not. Happy training!",
                textAlign: TextAlign.center,
                style: TextStyle(fontSize: 16),
              ),
            ),
            const SizedBox(height: 40),
            if (started)
              AnimatedSwitcher(
                duration: const Duration(milliseconds: 500),
                child: Text(
                  instruction,
                  key: ValueKey(instruction + DateTime.now().toString()),
                  style: const TextStyle(fontSize: 30, fontWeight: FontWeight.bold),
                ),
              ),
            const SizedBox(height: 30),
            ElevatedButton(
              onPressed: () {
                if (!started) {
                  setState(() => started = true);
                  generateInstruction();
                } else {
                  generateInstruction();
                }
              },
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.symmetric(horizontal: 40, vertical: 15),
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
              ),
              child: Text(started ? 'Again' : 'Start'),
            ),
            const SizedBox(height: 20),
            if (result.isNotEmpty)
              Text(
                result,
                style: TextStyle(
                  fontSize: 20,
                  fontWeight: FontWeight.bold,
                  color: result == "Your shot is correct!"
                      ? Colors.green
                      : result == "Your shot is not correct!"
                      ? Colors.red
                      : const Color(0xFFB8860B), // dark yellow (goldenrod)
                ),
              ),
          ],
        ),
      ),
    );
  }
}