//
// unsigned int metric = 0;
// const unsigned int metric_critical = 100; // мб при energy-saving менять это дело
//
// void upd_metric() {
//     unsigned int T_metric = abs(round( 100 * (get_target_temp() - get_curr_temp()) ));
//     unsigned int CO2_metric = 2 * max(0, get_curr_CO2() - get_target_CO2());
//
//     metric = T_metric + CO2_metric;
// }
//
// bool is_metric_ok() {
//     return (metric < metric_critical);
// }
