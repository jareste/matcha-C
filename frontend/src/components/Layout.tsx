import { Outlet } from 'react-router'
import Footer from './Footer.tsx'
import Header from './Header.tsx'

function Layout() {
  return (
    <div className="flex min-h-svh flex-col bg-white text-gray-800 dark:bg-gray-950 dark:text-gray-200">
      <Header />
      <main className="mx-auto flex w-full max-w-5xl flex-1 flex-col px-4 py-8 sm:px-6">
        <Outlet />
      </main>
      <Footer />
    </div>
  )
}

export default Layout
