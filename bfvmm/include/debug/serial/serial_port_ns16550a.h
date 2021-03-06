//
// Bareflank Hypervisor
// Copyright (C) 2015 Assured Information Security, Inc.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

#ifndef SERIAL_PORT_NS16550A_H
#define SERIAL_PORT_NS16550A_H

#include <string>
#include <memory>

#include <bfconstants.h>

#include <intrinsics.h>
#include "serial_port_base.h"

// -----------------------------------------------------------------------------
// Exports
// -----------------------------------------------------------------------------

#include <bfexports.h>

#ifndef STATIC_DEBUG
#ifdef SHARED_DEBUG
#define EXPORT_DEBUG EXPORT_SYM
#else
#define EXPORT_DEBUG IMPORT_SYM
#endif
#else
#define EXPORT_DEBUG
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

// -----------------------------------------------------------------------------
// Constants
// -----------------------------------------------------------------------------

/// @cond

namespace bfvmm
{

namespace serial_ns16550a
{

constexpr const serial_port_base::value_type_8 dlab = 1U << 7;

constexpr const serial_port_base::port_type baud_rate_lo_reg = 0U;
constexpr const serial_port_base::port_type baud_rate_hi_reg = 1U;
constexpr const serial_port_base::port_type interrupt_en_reg = 1U;
constexpr const serial_port_base::port_type fifo_control_reg = 2U;
constexpr const serial_port_base::port_type line_control_reg = 3U;
constexpr const serial_port_base::port_type line_status_reg = 5U;

constexpr const serial_port_base::value_type_8 fifo_control_enable_fifos = 1U << 0;
constexpr const serial_port_base::value_type_8 fifo_control_clear_recieve_fifo = 1U << 1;
constexpr const serial_port_base::value_type_8 fifo_control_clear_transmit_fifo = 1U << 2;
constexpr const serial_port_base::value_type_8 fifo_control_dma_mode_select = 1U << 3;

constexpr const serial_port_base::value_type_8 line_status_data_ready = 1U << 0;
constexpr const serial_port_base::value_type_8 line_status_overrun_error = 1U << 1;
constexpr const serial_port_base::value_type_8 line_status_parity_error = 1U << 2;
constexpr const serial_port_base::value_type_8 line_status_framing_error = 1U << 3;
constexpr const serial_port_base::value_type_8 line_status_break_interrupt = 1U << 4;
constexpr const serial_port_base::value_type_8 line_status_empty_transmitter = 1U << 5;
constexpr const serial_port_base::value_type_8 line_status_empty_data = 1U << 6;
constexpr const serial_port_base::value_type_8 line_status_recieved_fifo_error = 1U << 7;

constexpr const serial_port_base::value_type_8 line_control_data_mask = 0x03;
constexpr const serial_port_base::value_type_8 line_control_stop_mask = 0x04;
constexpr const serial_port_base::value_type_8 line_control_parity_mask = 0x38;

}

/// @endcond

// -----------------------------------------------------------------------------
// Definitions
// -----------------------------------------------------------------------------

/// Serial Port (NatSemi 16550A and compatible)
///
/// This class implements the serial device for Intel specific archiectures.
/// All of the serial devices start off with the same default settings (minus
/// the port). There are no checks on the port # (in case a custom port number
/// is needed), and there are no checks to ensure that only one port is used
/// at a time. If custom port settings are required, once the serial port is
/// created, the custom settings can be setup by using the set_xxx functions.
/// The user should ensure that the settings worked by checking the result.
///
/// Also note, that by default, a FIFO is used / required, and interrupts are
/// disabled.
///
class EXPORT_DEBUG serial_port_ns16550a : public serial_port_base
{
public:

    /// @cond

    enum baud_rate_t {
        baud_rate_50 = 0x0900,
        baud_rate_75 = 0x0600,
        baud_rate_110 = 0x0417,
        baud_rate_150 = 0x0300,
        baud_rate_300 = 0x0180,
        baud_rate_600 = 0x00C0,
        baud_rate_1200 = 0x0060,
        baud_rate_1800 = 0x0040,
        baud_rate_2000 = 0x003A,
        baud_rate_2400 = 0x0030,
        baud_rate_3600 = 0x0020,
        baud_rate_4800 = 0x0018,
        baud_rate_7200 = 0x0010,
        baud_rate_9600 = 0x000C,
        baud_rate_19200 = 0x0006,
        baud_rate_38400 = 0x0003,
        baud_rate_57600 = 0x0002,
        baud_rate_115200 = 0x0001
    };

    enum data_bits_t {
        char_length_5 = 0x00,
        char_length_6 = 0x01,
        char_length_7 = 0x02,
        char_length_8 = 0x03
    };

    enum stop_bits_t {
        stop_bits_1 = 0x00,
        stop_bits_2 = 0x04
    };

    enum parity_bits_t {
        parity_none = 0x00,
        parity_odd = 0x08,
        parity_even = 0x18,
        parity_mark = 0x28,
        parity_space = 0x38
    };

    /// @endcond

public:

    /// Default constructor - uses the default port
    ///
    /// @expects none
    /// @ensures none
    ///
    serial_port_ns16550a() noexcept;

    /// Specific constructor - accepts a target port address
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param port the serial port to connect to
    ///
    serial_port_ns16550a(port_type port) noexcept;

    /// Destructor
    ///
    /// @expects none
    /// @ensures none
    ///
    ~serial_port_ns16550a() = default;

    /// Get Instance
    ///
    /// Get an instance to the class.
    ///
    /// @expects none
    /// @ensures ret != nullptr
    ///
    /// @return a singleton instance of serial_port_ns16550a
    ///
    static serial_port_ns16550a *instance() noexcept;

    /// Set Baud Rate
    ///
    /// Sets the rate at which the serial device will operate. Note that the
    /// rate paramter is actually the divisor that is used, and a custom one
    /// can be used if desired. If 0 is provided, the default baud rate is
    /// used instead.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param rate desired baud rate
    ///
    void set_baud_rate(baud_rate_t rate) noexcept;

    /// Buad Rate
    ///
    /// Returns the baud rate of the serial device. If the serial device is
    /// set to a baud rate that this code does not recognize, unknown is
    /// returned.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the baud rate
    ///
    baud_rate_t baud_rate() const noexcept;

    /// Set Data Bits
    ///
    /// Sets the size of the data that is transmitted. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired data bits
    ///
    void set_data_bits(data_bits_t bits) noexcept;

    /// Data Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's data bits
    ///
    data_bits_t data_bits() const noexcept;

    /// Set Stop Bits
    ///
    /// Sets the stop bits used for transmission. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired stop bits
    ///
    void set_stop_bits(stop_bits_t bits) noexcept;

    /// Stop Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's stop bits
    ///
    stop_bits_t stop_bits() const noexcept;

    /// Set Parity Bits
    ///
    /// Sets the parity bits used for transmission. For more information
    /// on the this field, please see http://wiki.osdev.org/Serial_Ports.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param bits the desired parity bits
    ///
    void set_parity_bits(parity_bits_t bits) noexcept;

    /// Parity Bits
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's parity bits
    ///
    parity_bits_t parity_bits() const noexcept;

    /// Port
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @return the serial device's port
    ///
    virtual port_type port() const noexcept override
    { return m_port; }

    /// Set Port
    ///
    /// Change the peripheral port/base address at runtime.
    ///
    /// @param port serial peripheral port or base address
    ///
    /// @expects none
    /// @ensures none
    ///
    virtual void set_port(port_type port) noexcept override
    {
        m_port = port;
    }

    /// Write Character
    ///
    /// Writes a character to the serial device.
    ///
    /// @expects none
    /// @ensures none
    ///
    /// @param c character to write
    ///
    virtual void write(char c) noexcept override;

    using serial_port_base::write;

private:

    void enable_dlab() const noexcept;
    void disable_dlab() const noexcept;

    bool get_line_status_empty_transmitter() const noexcept;

    void init(port_type port) noexcept;

    port_type m_port;

public:

    /// @cond

    serial_port_ns16550a(serial_port_ns16550a &&) noexcept = default;
    serial_port_ns16550a &operator=(serial_port_ns16550a &&) noexcept = default;

    serial_port_ns16550a(const serial_port_ns16550a &) = delete;
    serial_port_ns16550a &operator=(const serial_port_ns16550a &) = delete;

    /// @endcond
};

}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
