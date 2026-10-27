library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity i2c_mpu6050_fsm is
    port (
        clk          : in    std_logic;
        rst          : in    std_logic;
        tick_400k    : in    std_logic;
        i2c_scl      : inout std_logic;
        i2c_sda      : inout std_logic;
        accel_data   : out   std_logic_vector(47 downto 0);
        sample_valid : out   std_logic;
        bus_ok       : out   std_logic;
        bus_err      : out   std_logic
    );
end entity i2c_mpu6050_fsm;

architecture rtl of i2c_mpu6050_fsm is
    type state_t is (
        ST_INIT_START, ST_SEND_BYTE, ST_WAIT_ACK,
        ST_WAKE_REG, ST_WAKE_VAL, ST_INIT_STOP,
        ST_READ_START, ST_REG_ADDR, ST_REP_START,
        ST_READ_BYTE, ST_SEND_MACK, ST_STOP, ST_GAP
    );

    signal state       : state_t := ST_INIT_START;
    signal phase       : unsigned(1 downto 0) := "00";
    signal bit_idx     : integer range 0 to 7 := 7;
    signal byte_cnt    : integer range 0 to 5 := 0;
    signal step_id     : integer range 0 to 5 := 0;

    signal scl_low     : std_logic := '0';
    signal sda_low     : std_logic := '0';
    signal tx_byte     : std_logic_vector(7 downto 0) := x"00";
    signal rx_shift    : std_logic_vector(7 downto 0) := x"00";
    signal shift_buf   : std_logic_vector(47 downto 0) := (others => '0');
    signal ack_ok      : std_logic := '1';
    signal gap_cnt     : unsigned(12 downto 0) := (others => '0');
begin
    -- Open-drain I2C physical drivers
    i2c_scl <= '0' when scl_low = '1' else 'Z';
    i2c_sda <= '0' when sda_low = '1' else 'Z';

    process(clk)
    begin
        if rising_edge(clk) then
            sample_valid <= '0';

            if rst = '1' then
                state    <= ST_INIT_START;
                scl_low  <= '0';
                sda_low  <= '0';
                ack_ok   <= '1';
                bus_ok   <= '0';
                bus_err  <= '0';
            elsif tick_400k = '1' then
                case state is
                    -- 1. Wake up MPU6050: Write 0x00 to PWR_MGMT_1 (0x6B)
                    when ST_INIT_START =>
                        case phase is
                            when "00" =>
                                scl_low <= '0';
                                sda_low <= '0';
                                ack_ok  <= '1';
                                phase   <= "01";
                            when "01" => sda_low <= '1'; phase <= "10";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                tx_byte <= x"D0"; bit_idx <= 7; step_id <= 0;
                                state   <= ST_SEND_BYTE;
                        end case;

                    when ST_SEND_BYTE =>
                        case phase is
                            when "00" =>
                                scl_low <= '1';
                                if tx_byte(bit_idx) = '0' then sda_low <= '1'; else sda_low <= '0'; end if;
                                phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when "10" => phase <= "11";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                if bit_idx = 0 then state <= ST_WAIT_ACK;
                                else bit_idx <= bit_idx - 1; end if;
                        end case;

                    when ST_WAIT_ACK =>
                        case phase is
                            when "00" => scl_low <= '1'; sda_low <= '0'; phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when "10" =>
                                -- Keep any NACK sticky through the whole wake/read cycle.
                                if i2c_sda /= '0' then ack_ok <= '0'; end if;
                                phase <= "11";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                case step_id is
                                    when 0 => state <= ST_WAKE_REG;
                                    when 1 => state <= ST_WAKE_VAL;
                                    when 2 => state <= ST_INIT_STOP;
                                    when 3 => state <= ST_REG_ADDR;
                                    when 4 => state <= ST_REP_START;
                                    when others => bit_idx <= 7; byte_cnt <= 0; state <= ST_READ_BYTE;
                                end case;
                        end case;

                    when ST_WAKE_REG =>
                        tx_byte <= x"6B"; bit_idx <= 7; step_id <= 1; state <= ST_SEND_BYTE;

                    when ST_WAKE_VAL =>
                        tx_byte <= x"00"; bit_idx <= 7; step_id <= 2; state <= ST_SEND_BYTE;

                    when ST_INIT_STOP =>
                        case phase is
                            when "00" => scl_low <= '1'; sda_low <= '1'; phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when others => sda_low <= '0'; phase <= "00"; state <= ST_READ_START;
                        end case;

                    -- 2. Burst-Read 6 Accelerometer Registers (0x3B to 0x40)
                    when ST_READ_START =>
                        case phase is
                            when "00" => scl_low <= '0'; sda_low <= '0'; phase <= "01";
                            when "01" => sda_low <= '1'; phase <= "10";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                tx_byte <= x"D0"; bit_idx <= 7; step_id <= 3;
                                state   <= ST_SEND_BYTE;
                        end case;

                    when ST_REG_ADDR =>
                        tx_byte <= x"3B"; bit_idx <= 7; step_id <= 4; state <= ST_SEND_BYTE;

                    when ST_REP_START =>
                        case phase is
                            when "00" => scl_low <= '1'; sda_low <= '0'; phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when "10" => sda_low <= '1'; phase <= "11";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                tx_byte <= x"D1"; bit_idx <= 7; step_id <= 5;
                                state   <= ST_SEND_BYTE;
                        end case;

                    when ST_READ_BYTE =>
                        case phase is
                            when "00" => scl_low <= '1'; sda_low <= '0'; phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when "10" =>
                                if i2c_sda = '0' then rx_shift(bit_idx) <= '0'; else rx_shift(bit_idx) <= '1'; end if;
                                phase <= "11";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                if bit_idx = 0 then
                                    shift_buf(47 - byte_cnt*8 downto 40 - byte_cnt*8) <= rx_shift;
                                    state <= ST_SEND_MACK;
                                else
                                    bit_idx <= bit_idx - 1;
                                end if;
                        end case;

                    when ST_SEND_MACK =>
                        case phase is
                            when "00" =>
                                scl_low <= '1';
                                -- Master sends ACK ('0') for bytes 0..4, NACK ('Z') on last byte 5
                                if byte_cnt < 5 then sda_low <= '1'; else sda_low <= '0'; end if;
                                phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when "10" => phase <= "11";
                            when others =>
                                scl_low <= '1'; phase <= "00";
                                if byte_cnt < 5 then
                                    byte_cnt <= byte_cnt + 1;
                                    bit_idx  <= 7;
                                    state    <= ST_READ_BYTE;
                                else
                                    state    <= ST_STOP;
                                end if;
                        end case;

                    when ST_STOP =>
                        case phase is
                            when "00" => scl_low <= '1'; sda_low <= '1'; phase <= "01";
                            when "01" => scl_low <= '0'; phase <= "10";
                            when others =>
                                sda_low      <= '0';
                                phase        <= "00";
                                if ack_ok = '1' then
                                    accel_data   <= shift_buf;
                                    sample_valid <= '1';
                                end if;
                                bus_ok       <= ack_ok;
                                bus_err      <= not ack_ok;
                                gap_cnt      <= (others => '0');
                                state        <= ST_GAP;
                        end case;

                   when ST_GAP =>
                        if gap_cnt = to_unsigned(6000, 13) then
                            gap_cnt <= (others => '0');
                            if ack_ok = '1' then
                                state <= ST_READ_START;
                            else
                                state <= ST_INIT_START; -- Auto-retry the wake command if sensor was asleep!
                            end if;
                        else
                            gap_cnt <= gap_cnt + 1;
                        end if;
                end case;
            end if;
        end if;
    end process;
end architecture rtl;
