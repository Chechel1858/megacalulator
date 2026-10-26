// Панель оператора: список заказов и смена их статуса.

const ordersBody = document.getElementById('orders');
const noOrders = document.getElementById('no-orders');
const adminError = document.getElementById('admin-error');

function createStatusSelect(order) {
    const select = createElement('select', `badge badge--${order.status}`);
    for (const [code, name] of Object.entries(STATUS_NAMES)) {
        const option = createElement('option', '', name);
        option.value = code;
        option.selected = code === order.status;
        select.append(option);
    }
    select.addEventListener('change', async () => {
        select.disabled = true;
        try {
            const updated = await api(`/api/orders/${order.id}/status`, {
                method: 'PATCH',
                body: JSON.stringify({ status: select.value }),
            });
            select.className = `badge badge--${updated.status}`;
            order.status = updated.status;
        } catch (error) {
            select.value = order.status;
            showAlert(adminError, error.message);
        } finally {
            select.disabled = false;
        }
    });
    return select;
}

function renderOrders(orders) {
    ordersBody.replaceChildren();
    noOrders.hidden = orders.length > 0;

    for (const order of [...orders].reverse()) {
        const row = document.createElement('tr');

        const customer = createElement('td');
        customer.append(createElement('div', '', order.customer_name),
            createElement('div', 'muted', order.phone));

        const address = createElement('td');
        address.append(createElement('div', '', order.address));
        if (order.comment) address.append(createElement('div', 'muted', `💬 ${order.comment}`));

        const items = createElement('td');
        for (const item of order.items) {
            items.append(createElement('div', '', `${item.product_name} × ${item.quantity}`));
        }

        const status = createElement('td');
        status.append(createStatusSelect(order));

        row.append(
            createElement('td', '', String(order.id)),
            createElement('td', '', order.created_at),
            customer,
            address,
            createElement('td', '', formatDate(order.delivery_date)),
            items,
            createElement('td', '', formatPrice(order.total)),
            status,
        );
        ordersBody.append(row);
    }
}

async function loadOrders() {
    // Не перерисовываем таблицу, пока оператор выбирает статус.
    if (document.activeElement && document.activeElement.tagName === 'SELECT') return;
    try {
        renderOrders(await api('/api/orders'));
        adminError.hidden = true;
    } catch (error) {
        showAlert(adminError, error.message);
    }
}

document.getElementById('refresh').addEventListener('click', loadOrders);
loadOrders();
setInterval(loadOrders, 10000);
