// Сайт клиента: каталог, корзина, оформление заказа и проверка статуса.

const cart = new Map(); // product_id -> количество
let products = [];

const elements = {
    serverError: document.getElementById('server-error'),
    products: document.getElementById('products'),
    cartItems: document.getElementById('cart-items'),
    cartEmpty: document.getElementById('cart-empty'),
    cartTotal: document.getElementById('cart-total'),
    orderForm: document.getElementById('order-form'),
    formErrors: document.getElementById('form-errors'),
    submitButton: document.getElementById('submit-button'),
    orderSuccess: document.getElementById('order-success'),
    statusForm: document.getElementById('status-form'),
    statusResult: document.getElementById('status-result'),
};

function todayIso() {
    const now = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    return `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
}

function setQuantity(productId, quantity) {
    const value = Math.max(0, Math.min(100, quantity));
    if (value === 0) {
        cart.delete(productId);
    } else {
        cart.set(productId, value);
    }
    renderProducts();
    renderCart();
}

function renderProducts() {
    elements.products.replaceChildren();
    for (const product of products) {
        const card = createElement('article', 'card product');
        card.append(
            createElement('div', 'product__icon', product.id === 5 ? '🚰' : '💧'),
            createElement('h3', 'product__name', product.name),
            createElement('p', 'product__description muted', product.description),
            createElement('div', 'product__price', formatPrice(product.price)),
        );

        const quantity = cart.get(product.id) || 0;
        const controls = createElement('div', 'product__controls');
        if (quantity === 0) {
            const add = createElement('button', 'button button--primary', 'В корзину');
            add.type = 'button';
            add.addEventListener('click', () => setQuantity(product.id, 1));
            controls.append(add);
        } else {
            const minus = createElement('button', 'button button--round', '−');
            const plus = createElement('button', 'button button--round', '+');
            minus.type = plus.type = 'button';
            minus.addEventListener('click', () => setQuantity(product.id, quantity - 1));
            plus.addEventListener('click', () => setQuantity(product.id, quantity + 1));
            controls.append(minus, createElement('span', 'product__quantity', `${quantity} шт.`), plus);
        }
        card.append(controls);
        elements.products.append(card);
    }
}

function renderCart() {
    elements.cartItems.replaceChildren();
    let total = 0;
    for (const [productId, quantity] of cart) {
        const product = products.find((p) => p.id === productId);
        const sum = product.price * quantity;
        total += sum;
        const item = createElement('li', 'cart__item');
        item.append(
            createElement('span', '', `${product.name} × ${quantity}`),
            createElement('span', '', formatPrice(sum)),
        );
        elements.cartItems.append(item);
    }
    elements.cartEmpty.hidden = cart.size > 0;
    elements.cartTotal.textContent = formatPrice(total);
}

async function loadProducts() {
    try {
        products = await api('/api/products');
        renderProducts();
        renderCart();
    } catch (error) {
        elements.products.replaceChildren();
        showAlert(elements.serverError, error.message);
    }
}

async function submitOrder(event) {
    event.preventDefault();
    elements.formErrors.hidden = true;
    elements.orderSuccess.hidden = true;

    if (cart.size === 0) {
        showAlert(elements.formErrors, 'Добавьте в заказ хотя бы один товар');
        return;
    }

    const form = new FormData(elements.orderForm);
    const order = {
        customer_name: form.get('customer_name'),
        phone: form.get('phone'),
        address: form.get('address'),
        delivery_date: form.get('delivery_date'),
        comment: form.get('comment'),
        items: [...cart].map(([product_id, quantity]) => ({ product_id, quantity })),
    };

    elements.submitButton.disabled = true;
    try {
        const saved = await api('/api/orders', { method: 'POST', body: JSON.stringify(order) });
        showAlert(elements.orderSuccess, [
            `Заказ №${saved.id} принят!`,
            `Сумма: ${formatPrice(saved.total)}. Доставка ${formatDate(saved.delivery_date)} `
                + `по адресу: ${saved.address}.`,
            'Сохраните номер заказа, чтобы проверить его статус.',
        ]);
        elements.orderSuccess.scrollIntoView({ behavior: 'smooth', block: 'center' });
        elements.orderForm.reset();
        elements.orderForm.delivery_date.value = todayIso();
        cart.clear();
        renderProducts();
        renderCart();
    } catch (error) {
        showAlert(elements.formErrors, error.errors || error.message);
    } finally {
        elements.submitButton.disabled = false;
    }
}

async function checkStatus(event) {
    event.preventDefault();
    const id = new FormData(elements.statusForm).get('id');
    elements.statusResult.replaceChildren();
    try {
        const order = await api(`/api/orders/${encodeURIComponent(id)}`);
        const card = createElement('div', 'card status-card');
        card.append(
            createElement('h3', '', `Заказ №${order.id}`),
            createElement('span', `badge badge--${order.status}`, STATUS_NAMES[order.status]),
            createElement('p', '', `Дата доставки: ${formatDate(order.delivery_date)}`),
            createElement('p', '', `Сумма: ${formatPrice(order.total)}`),
        );
        elements.statusResult.append(card);
    } catch (error) {
        elements.statusResult.append(createElement('p', 'alert alert--error', error.message));
    }
}

elements.orderForm.delivery_date.min = todayIso();
elements.orderForm.delivery_date.value = todayIso();
elements.orderForm.addEventListener('submit', submitOrder);
elements.statusForm.addEventListener('submit', checkStatus);
loadProducts();
