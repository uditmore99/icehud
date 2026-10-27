library ieee;
use ieee.std_logic_1164.all;

entity hud_top is
    port (
        clk     : in    std_logic;
        i2c_scl : inout std_logic;
        i2c_sda : inout std_logic;
        uart_tx : out   std_logic;
        led_r_n : out   std_logic;
        led_g_n : out   std_logic;
        led_b_n : out   std_logic
    );
end entity hud_top;

architecture rtl of hud_top is
    signal tick_400k    : std_logic;
    signal tick_115k    : std_logic;
    signal accel_vec    : std_logic_vector(47 downto 0);
    signal sample_rdy   : std_logic;
    signal ok_flag      : std_logic;
    signal err_flag     : std_logic;
    signal uart_busy    : std_logic;
begin
    u_clk_div : entity work.clk_divider
        port map (
            clk            => clk,
            rst            => '0',
            i2c_tick_400k  => tick_400k,
            uart_tick_115k => tick_115k
        );

    u_i2c_fsm : entity work.i2c_mpu6050_fsm
        port map (
            clk          => clk,
            rst          => '0',
            tick_400k    => tick_400k,
            i2c_scl      => i2c_scl,
            i2c_sda      => i2c_sda,
            accel_data   => accel_vec,
            sample_valid => sample_rdy,
            bus_ok       => ok_flag,
            bus_err      => err_flag
        );

    u_uart_tx : entity work.uart_tx
        port map (
            clk          => clk,
            rst          => '0',
            baud_tick    => tick_115k,
            start_packet => sample_rdy,
            accel_in     => accel_vec,
            tx_pin       => uart_tx,
            tx_busy      => uart_busy
        );

    -- Active-low onboard RGB LEDs
    led_g_n <= '0' when ok_flag   = '1' else '1';
    led_r_n <= '0' when err_flag  = '1' else '1';
    led_b_n <= '0' when uart_busy = '1' else '1';
end architecture rtl;