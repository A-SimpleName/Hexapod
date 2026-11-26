package com.example.app

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.location.LocationManager
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.net.wifi.WifiManager
import android.net.wifi.WifiNetworkSpecifier
import android.os.Build
import android.os.Handler
import android.os.Looper
import io.flutter.plugin.common.BinaryMessenger
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetSocketAddress
import java.util.concurrent.Executors

internal class RobotWifi(
    private val activity: Activity,
    messenger: BinaryMessenger
) {
    // The shared workshop credentials match the ESP32 firmware.
    private companion object {
        const val SSID = "Hexapod-ESP32"
        const val PASSPHRASE = "hexapod123"
        const val PERMISSION_REQUEST = 4210
    }

    private data class Join(
        val host: String,
        val port: Int,
        val result: MethodChannel.Result
    )

    private val connectivity = checkNotNull(activity.getSystemService(ConnectivityManager::class.java))
    private val wifi = activity.applicationContext.getSystemService(WifiManager::class.java)
    private val channel = MethodChannel(messenger, "hexapod/wifi")
    private val mainThread = Handler(Looper.getMainLooper())
    private val socketWorker = Executors.newSingleThreadExecutor()
    private var pendingJoin: Join? = null
    private var callback: ConnectivityManager.NetworkCallback? = null
    private var robotNetwork: Network? = null
    private var robotSocket: DatagramSocket? = null

    init {
        channel.setMethodCallHandler { call, result ->
            when (call.method) {
                "connect" -> connect(call, result)
                "send" -> send(call.arguments as? ByteArray, result)
                "disconnect" -> {
                    release()
                    result.success(null)
                }
                else -> result.notImplemented()
            }
        }
    }

    private fun connect(call: MethodCall, result: MethodChannel.Result) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            result.error("unsupported", "In-app Wi-Fi needs Android 10 or newer.", null)
            return
        }
        if (pendingJoin != null) {
            result.error("busy", "Already connecting to the robot.", null)
            return
        }
        if (robotSocket != null) {
            result.success(null)
            return
        }
        if (wifi?.isWifiEnabled != true) {
            result.error("wifi_off", "Turn on Wi-Fi and try again.", null)
            return
        }
        val host = call.argument<String>("host")
        val port = call.argument<Int>("port")
        if (host.isNullOrEmpty() || port == null || port !in 1..65535) {
            result.error("invalid_address", "The robot's UDP address is invalid.", null)
            return
        }

        pendingJoin = Join(host, port, result)
        val permission = wifiPermission()
        if (activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED) {
            requestRobotNetwork()
        } else {
            val permissions = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                arrayOf(permission)
            } else {
                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)
            }
            activity.requestPermissions(permissions, PERMISSION_REQUEST)
        }
    }

    private fun wifiPermission(): String =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.NEARBY_WIFI_DEVICES
        } else {
            Manifest.permission.ACCESS_FINE_LOCATION
        }

    fun onRequestPermissionsResult(requestCode: Int) {
        if (requestCode != PERMISSION_REQUEST || pendingJoin == null) return
        if (activity.checkSelfPermission(wifiPermission()) == PackageManager.PERMISSION_GRANTED) {
            requestRobotNetwork()
        } else {
            failJoin("permission_denied", "Wi-Fi permission is needed to connect to the robot.")
        }
    }

    private fun requestRobotNetwork() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) return
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU &&
            activity.getSystemService(LocationManager::class.java)?.isLocationEnabled != true
        ) {
            failJoin("location_off", "Turn on Location to find Wi-Fi devices on this version of Android.")
            return
        }

        val request = NetworkRequest.Builder()
            .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
            .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .setNetworkSpecifier(
                WifiNetworkSpecifier.Builder().setSsid(SSID).setWpa2Passphrase(PASSPHRASE).build()
            )
            .build()
        val networkCallback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                val join = pendingJoin ?: return
                if (callback !== this || robotNetwork == network) return
                robotNetwork = network
                openSocket(network, join)
            }

            override fun onUnavailable() {
                if (callback === this) {
                    failJoin("unavailable", "Could not join $SSID. Check that the robot is on and try again.")
                }
            }

            override fun onLost(network: Network) {
                if (callback !== this || robotNetwork != network) return
                if (pendingJoin != null) {
                    failJoin("lost", "The robot's Wi-Fi connection dropped. Try again.")
                } else {
                    release()
                    channel.invokeMethod("disconnected", null)
                }
            }
        }
        callback = networkCallback
        try {
            connectivity.requestNetwork(request, networkCallback, mainThread, 45_000)
        } catch (error: SecurityException) {
            failJoin("permission_denied", "Android denied the Wi-Fi request. Check app permissions.")
        } catch (error: RuntimeException) {
            failJoin("unavailable", "Android could not start the Wi-Fi request. Try again.")
        }
    }

    private fun openSocket(network: Network, join: Join) {
        socketWorker.execute {
            var socket: DatagramSocket? = null
            try {
                socket = DatagramSocket()
                network.bindSocket(socket)
                socket.connect(InetSocketAddress(join.host, join.port))
            } catch (error: Exception) {
                socket?.close()
                socket = null
            }
            val opened = socket
            mainThread.post {
                if (pendingJoin !== join || robotNetwork != network) {
                    opened?.close()
                } else if (opened == null) {
                    failJoin("socket_error", "Joined Wi-Fi, but could not open a connection to the robot.")
                } else {
                    robotSocket = opened
                    pendingJoin = null
                    join.result.success(null)
                }
            }
        }
    }

    private fun send(data: ByteArray?, result: MethodChannel.Result) {
        val socket = robotSocket
        if (data == null || socket == null) {
            result.success(false)
            return
        }
        socketWorker.execute {
            result.success(runCatching {
                socket.send(DatagramPacket(data, data.size))
                true
            }.getOrDefault(false))
        }
    }

    private fun failJoin(code: String, message: String) {
        val join = pendingJoin
        pendingJoin = null
        release()
        join?.result?.error(code, message, null)
    }

    private fun release() {
        pendingJoin?.result?.error("cancelled", "Wi-Fi connection cancelled.", null)
        pendingJoin = null
        val registered = callback
        callback = null
        robotNetwork = null
        robotSocket?.close()
        robotSocket = null
        if (registered != null) {
            // A timed-out request may already have been removed by Android.
            runCatching { connectivity.unregisterNetworkCallback(registered) }
        }
    }

    fun close() {
        channel.setMethodCallHandler(null)
        release()
        socketWorker.shutdown()
    }
}
