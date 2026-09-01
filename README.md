Reads 24 metrics (currently) and stores in memory for up to 30 days. Currently no persistence or database. Accepts http requests and returns json format to be accepted by dataviz dashboards like Grafana with json plugins.
```
metrics_count = {
    "cpu_util",
    "mem_total", "mem_free", "mem_available", "swap_total", "swap_free",
    "minute_load", "running_procs", "total_procs",
    "rx_bytesps", "tx_bytesps", "rx_packetsps", "tx_packetsps", "rx_dropsps", "tx_dropsps",
    "read_bytesps", "write_bytesps", "io_msps",
    "uptime",
    "total_bytes", "free_bytes", "avail_bytes", "total_inodes", "free_inodes"
};
```
